# Copyright 2018 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
"""Statistical test assertions calibrated for their error rates."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import itertools

from tensorflow.python.framework import ops
from tensorflow.python.ops import array_ops
from tensorflow.python.ops import check_ops
from tensorflow.python.ops import control_flow_ops
from tensorflow.python.ops import gen_math_ops
from tensorflow.python.ops import math_ops
from tensorflow.python.ops import nn_ops

__all__ = [
    "true_mean_confidence_interval_by_dkwm",
    "assert_true_mean_equal_by_dkwm",
    "min_discrepancy_of_true_means_detectable_by_dkwm",
    "min_num_samples_for_dkwm_mean_test",
    "assert_true_mean_equal_by_dkwm_two_sample",
    "min_discrepancy_of_true_means_detectable_by_dkwm_two_sample",
    "min_num_samples_for_dkwm_mean_two_sample_test",
]


def _batch_sort_vector(x, ascending=True, name=None):
  with ops.name_scope(name, "sort_each_row", [x]):
    x = ops.convert_to_tensor(x, name="x")
    n = array_ops.shape(x)[-1]
    if ascending:
      y, _ = nn_ops.top_k(-x, k=n, sorted=True)
      y = -y
    else:
      y, _ = nn_ops.top_k(x, k=n, sorted=True)
    y.set_shape(x.shape)
    return y


def _do_maximum_mean(samples, envelope, high, name=None):
  """Common code between maximum_mean and minimum_mean."""
  with ops.name_scope(name, "do_maximum_mean", [samples, envelope, high]):
    n = array_ops.rank(samples)
    # Move the batch dimension of `samples` to the rightmost position,
    # where the _batch_sort_vector function wants it.
    perm = array_ops.concat([math_ops.range(1, n), [0]], axis=0)
    samples = array_ops.transpose(samples, perm)

    samples = _batch_sort_vector(samples)
    batch_shape = array_ops.shape(samples)[:-1]
    n = array_ops.shape(samples)[-1]
    step = 1. / math_ops.cast(n, dtype=samples.dtype.base_dtype)

    def _loop_body(iter_, total, to_skip):
      total = array_ops.where(
          step <= to_skip,
          total,
          array_ops.where(
              to_skip > 0.,
              total + (step - to_skip) * samples[..., iter_],
              total + step * samples[..., iter_]))
      to_skip = array_ops.where(step <= to_skip, to_skip - step, 0.)
      return [iter_ + 1, total, to_skip]

    _, total, _ = control_flow_ops.while_loop(
        cond=lambda iter_, *args: iter_ < n,
        body=_loop_body,
        loop_vars=[
            0,
            array_ops.zeros(batch_shape, dtype=samples.dtype.base_dtype),
            envelope,  # to_skip
        ])

  return total + envelope * high


def _maximum_mean(samples, envelope, high, name=None):
  """Returns a stochastic upper bound on the mean of a scalar distribution.

  The idea is that if the true CDF is within an `eps`-envelope of the
  empirical CDF of the samples, and the support is bounded above, then
  the mean is bounded above as well.  In symbols,

  ```none
  sup_x(|F_n(x) - F(x)|) < eps
  ```

  The 0th dimension of `samples` is interpreted as independent and
  identically distributed samples.  The remaining dimensions are
  broadcast together with `envelope` and `high`, and operated on
  separately.

  Args:
    samples: Floating-point tensor of samples from the distribution(s)
      of interest.  Entries are assumed IID across the 0th dimension.
      The other dimensions must broadcast with `envelope` and `high`.
    envelope: Floating-point tensor of sizes of admissible CDF
      envelopes (i.e., the `eps` above).
    high: Floating-point tensor of upper bounds on the distributions'
      supports.
    name: A name for this operation (optional).

  Returns:
    bound: Floating-point tensor of upper bounds on the true means.

  Raises:
    InvalidArgumentError: If some `sample` is found to be larger than
      the corresponding `high`.
  """
  with ops.name_scope(name, "maximum_mean", [samples, envelope, high]):
    samples = ops.convert_to_tensor(samples, name="samples")
    envelope = ops.convert_to_tensor(envelope, name="envelope")
    high = ops.convert_to_tensor(high, name="high")

    xmax = math_ops.reduce_max(samples, axis=[-1])
    msg = "Given sample maximum value exceeds expectations"
    check_op = check_ops.assert_less_equal(xmax, high, message=msg)
    with ops.control_dependencies([check_op]):
      return array_ops.identity(_do_maximum_mean(samples, envelope, high))


def _minimum_mean(samples, envelope, low, name=None):
  """Returns a stochastic lower bound on the mean of a scalar distribution.

  The idea is that if the true CDF is within an `eps`-envelope of the
  empirical CDF of the samples, and the support is bounded below, then
  the mean is bounded below as well.  In symbols,

  ```none
  sup_x(|F_n(x) - F(x)|) < eps
  ```

  The 0th dimension of `samples` is interpreted as independent and
  identically distributed samples.  The remaining dimensions are
  broadcast together with `envelope` and `low`, and operated on
  separately.

  Args:
    samples: Floating-point tensor of samples from the distribution(s)
      of interest.  Entries are assumed IID across the 0th dimension.
      The other dimensions must broadcast with `envelope` and `low`.
    envelope: Floating-point tensor of sizes of admissible CDF
      envelopes (i.e., the `eps` above).
    low: Floating-point tensor of lower bounds on the distributions'
      supports.
    name: A name for this operation (optional).

  Returns:
    bound: Floating-point tensor of lower bounds on the true means.

  Raises:
    InvalidArgumentError: If some `sample` is found to be smaller than
      the corresponding `low`.
  """
  with ops.name_scope(name, "minimum_mean", [samples, envelope, low]):
    samples = ops.convert_to_tensor(samples, name="samples")
    envelope = ops.convert_to_tensor(envelope, name="envelope")
    low = ops.convert_to_tensor(low, name="low")

    xmin = math_ops.reduce_min(samples, axis=[-1])
    msg = "Given sample minimum value falls below expectations"
    check_op = check_ops.assert_greater_equal(xmin, low, message=msg)
    with ops.control_dependencies([check_op]):
      return - _do_maximum_mean(-samples, envelope, -low)


def _dkwm_cdf_envelope(n, error_rate, name=None):
  """Computes the CDF envelope that the DKWM inequality licenses.

  The [Dvoretzky-Kiefer-Wolfowitz-Massart inequality]
  (https://en.wikipedia.org/wiki/CDF-based_nonparametric_confidence_interval)
  gives a stochastic bound on the distance between the true cumulative
  distribution function (CDF) of any distribution and its empirical
  CDF.  To wit, for `n` iid samples from any distribution with CDF F,

  ```none
  P(sup_x |F_n(x) - F(x)| > eps) < 2exp(-2n eps^2)
  ```

  This function computes the envelope size `eps` as a function of the
  number of samples `n` and the desired limit on the left-hand
  probability above.

  Args:
    n: Tensor of numbers of samples drawn.
    error_rate: Floating-point tensor of admissible rates of mistakes.
    name: A name for this operation (optional).

  Returns:
    eps: Tensor of maximum distances the true CDF can be from the
      empirical CDF.  This scales as `O(sqrt(-log(error_rate)))` and
      as `O(1 / sqrt(n))`.  The shape is the broadcast of `n` and
      `error_rate`.
  """
  with ops.name_scope(name, "dkwm_cdf_envelope", [n, error_rate]):
    n = math_ops.cast(n, dtype=error_rate.dtype)
    return math_ops.sqrt(-gen_math_ops.log(error_rate / 2.) / (2. * n))


def _check_shape_dominates(tensor, tensors):
  """Check that broadcasting `tensor` against `tensors` does not expand it.

  Why?  Because I want to be very sure that the samples tensor is not
  accidentally enlarged by broadcasting against tensors that are
  supposed to be describing the distribution(s) sampled from, lest the
  sample counts end up inflated.

  Args:
    tensor: A Tensor whose shape is to be protected against broadcasting.
    tensors: A list of Tensors to check

  Returns:
    tensor: `tf.identity(tensor)` with control dependencies attached;
      be sure to use that downstream.
  """
  def check(t):
    target = array_ops.shape(tensor)[1:]
    result = array_ops.broadcast_dynamic_shape(target, array_ops.shape(t))
    # This rank check ensures that I don't get a wrong answer from the
    # _shapes_ broadcasting against each other.
    gt = check_ops.assert_greater(array_ops.rank(target), array_ops.rank(t))
    eq = check_ops.assert_equal(target, result)
    return gt, eq
  checks = list(itertools.chain(*[check(t) for t in tensors]))
  with ops.control_dependencies(checks):
    return array_ops.identity(array_ops.identity(tensor))


def true_mean_confidence_interval_by_dkwm(
    samples, low, high, error_rate=1e-6, name=None):
  """Computes a confidence interval for the mean of a scalar distribution.

  In batch mode, computes confidence intervals for all distributions
  in the batch (which need not be identically distributed).

  Relies on the [Dvoretzky-Kiefer-Wolfowitz-Massart inequality]
  (https://en.wikipedia.org/wiki/CDF-based_nonparametric_confidence_interval).

  The probability (over the randomness of drawing the given samples)
  that any true mean is outside the corresponding returned interval is
  no more than the given `error_rate`.  The size of the intervals
  scale as
  `O(1 / sqrt(#samples))`, as `O(high - low)`, and as `O(-log(error_rate))`.

  Note that `error_rate` is a total error rate for all the confidence
  intervals in the batch.  As such, if the batch is nontrivial, the
  error rate is not broadcast but divided (evenly) among the batch
  members.

  Args:
    samples: Floating-point tensor of samples from the distribution(s)
      of interest.  Entries are assumed IID across the 0th dimension.
      The other dimensions must broadcast with `low` and `high`.
    low: Floating-point tensor of lower bounds on the distributions'
      supports.
    high: Floating-point tensor of upper bounds on the distributions'
      supports.
    error_rate: *Scalar* admissible total rate of mistakes.
    name: A name for this operation (optional).

  Returns:
    low: A floating-point tensor of stochastic lower bounds on the true means.
    high: A floating-point tensor of stochastic upper bounds on the true means.
  """
  with ops.name_scope(
      name, "true_mean_confidence_interval_by_dkwm",
      [samples, low, high, error_rate]):
    samples = ops.convert_to_tensor(samples, name="samples")
    low = ops.convert_to_tensor(low, name="low")
    high = ops.convert_to_tensor(high, name="high")
    error_rate = ops.convert_to_tensor(error_rate, name="error_rate")
    samples = _check_shape_dominates(samples, [low, high])
    check_ops.assert_scalar(error_rate)  # Static shape
    error_rate = _itemwise_error_rate(error_rate, [low, high], samples)
    n = array_ops.shape(samples)[0]
    envelope = _dkwm_cdf_envelope(n, error_rate)
    min_mean = _minimum_mean(samples, envelope, low)
    max_mean = _maximum_mean(samples, envelope, high)
    return min_mean, max_mean


def _itemwise_error_rate(
    total_error_rate, param_tensors, sample_tensor=None, name=None):
  with ops.name_scope(
      name, "itemwise_error_rate",
      [total_error_rate, param_tensors, sample_tensor]):
    result_shape = [1]
    for p_tensor in param_tensors:
      result_shape = array_ops.broadcast_dynamic_shape(
          array_ops.shape(p_tensor), result_shape)
    if sample_tensor is not None:
      result_shape = array_ops.broadcast_dynamic_shape(
          array_ops.shape(sample_tensor)[1:], result_shape)
    num_items = math_ops.reduce_prod(result_shape)
    return total_error_rate / math_ops.cast(
        num_items, dtype=total_error_rate.dtype)


def assert_true_mean_equal_by_dkwm(
    samples, low, high, expected, false_fail_rate=1e-6, name=None):
  """Asserts the mean of the given distribution is as expected.

  More precisely, fails if there is enough evidence (using the
  [Dvoretzky-Kiefer-Wolfowitz-Massart inequality]
  (https://en.wikipedia.org/wiki/CDF-based_nonparametric_confidence_interval))
  that the true mean of some distribution from which the given samples are
  drawn is _not_ the given expected mean with statistical significance
  `false_fail_rate` or stronger, otherwise passes.  If you also want to
  check that you are gathering enough evidence that a pass is not
  spurious, see `min_num_samples_for_dkwm_mean_test` and
  `min_discrepancy_of_true_means_detectable_by_dkwm`.

  Note that `false_fail_rate` is a total false failure rate for all
  the assertions in the batch.  As such, if the batch is nontrivial,
  the assertion will insist on stronger evidence to fail any one member.

  Args:
    samples: Floating-point tensor of samples from the distribution(s)
      of interest.  Entries are assumed IID across the 0th dimension.
      The other dimensions must broadcast with `low` and `high`.
    low: Floating-point tensor of lower bounds on the distributions'
      supports.
    high: Floating-point tensor of upper bounds on the distributions'
      supports.
    expected: Floating-point tensor of expected true means.
    false_fail_rate: *Scalar* admissible total rate of mistakes.
    name: A name for this operation (optional).

  Returns:
    check: Op that raises `InvalidArgumentError` if any expected mean is
      outside the corresponding confidence interval.
  """
  with ops.name_scope(
      name, "assert_true_mean_equal_by_dkwm",
      [samples, low, high, expected, false_fail_rate]):
    samples = ops.convert_to_tensor(samples, name="samples")
    low = ops.convert_to_tensor(low, name="low")
    high = ops.convert_to_tensor(high, name="high")
    expected = ops.convert_to_tensor(expected, name="expected")
    false_fail_rate = ops.convert_to_tensor(
        false_fail_rate, name="false_fail_rate")
    samples = _check_shape_dominates(samples, [low, high, expected])
    min_mean, max_mean = true_mean_confidence_interval_by_dkwm(
        samples, low, high, error_rate=false_fail_rate)
    less_op = check_ops.assert_less(
        min_mean, expected, message="Mean confidence interval too high")
    with ops.control_dependencies([less_op]):
      return check_ops.assert_greater(
          max_mean, expected, message="Mean confidence interval too low")


def min_discrepancy_of_true_means_detectable_by_dkwm(
    n, low, high, false_fail_rate, false_pass_rate, name=None):
  """Returns the minimum mean discrepancy that a DKWM-based test can detect.

  DKWM is the [Dvoretzky-Kiefer-Wolfowitz-Massart inequality]
  (https://en.wikipedia.org/wiki/CDF-based_nonparametric_confidence_interval).

  Note that `false_fail_rate` is a total false failure rate for all
  the tests in the batch.  As such, if the batch is nontrivial, each
  member will demand more samples.  The `false_pass_rate` is also
  interpreted as a total, but is treated asymmetrically: If each test
  in the batch detects its corresponding discrepancy with probability
  at least `1 - false_pass_rate`, then running all those tests and
  failing if any one fails will jointly detect all those discrepancies
  with the same `false_pass_rate`.

  Args:
    n: Tensor of numbers of samples to be drawn from the distributions
      of interest.
    low: Floating-point tensor of lower bounds on the distributions'
      supports.
    high: Floating-point tensor of upper bounds on the distributions'
      supports.
    false_fail_rate: *Scalar* admissible total rate of false failures.
    false_pass_rate: *Scalar* admissible rate of false passes.
    name: A name for this operation (optional).

  Returns:
    discr: Tensor of lower bounds on the distances between true
       means detectable by a DKWM-based test.

  For each batch member `i`, of `K` total, drawing `n[i]` samples from
  some scalar distribution supported on `[low[i], high[i]]` is enough
  to detect a difference in means of size `discr[i]` or more.
  Specifically, we guarantee that (a) if the true mean is the expected
  mean, `assert_true_mean_equal_by_dkwm` will fail with probability at
  most `false_fail_rate / K` (which amounts to `false_fail_rate` if
  applied to the whole batch at once), and (b) if the true mean
  differs from the expected mean by at least `discr[i]`,
  `assert_true_mean_equal_by_dkwm` will pass with probability at most
  `false_pass_rate`.

  The detectable discrepancy scales as

  - `O(high[i] - low[i])`,
  - `O(1 / sqrt(n[i]))`,
  - `O(-log(false_fail_rate/K))`, and
  - `O(-log(false_pass_rate))`.
  """
  with ops.name_scope(
      name, "min_discrepancy_of_true_means_detectable_by_dkwm",
      [n, low, high, false_fail_rate, false_pass_rate]):
    n = ops.convert_to_tensor(n, name="n")
    low = ops.convert_to_tensor(low, name="low")
    high = ops.convert_to_tensor(high, name="high")
    false_fail_rate = ops.convert_to_tensor(
        false_fail_rate, name="false_fail_rate")
    false_pass_rate = ops.convert_to_tensor(
        false_pass_rate, name="false_pass_rate")
    # Algorithm: Assume a true CDF F.  The DKWM inequality gives a
    # stochastic bound on how far the observed empirical CDF F_n can be.
    # Then, using the DKWM inequality again gives a stochastic bound on
    # the farthest candidate true CDF F' that
    # true_mean_confidence_interval_by_dkwm might consider.  At worst, these
    # errors may go in the same direction, so the distance between F and
    # F' is bounded by the sum.
    # On batching: false fail rates sum, so I need to reduce
    # the input to account for the batching.  False pass rates
    # max, so I don't.
    sampling_envelope = _dkwm_cdf_envelope(n, false_pass_rate)
    false_fail_rate = _itemwise_error_rate(false_fail_rate, [n, low, high])
    analysis_envelope = _dkwm_cdf_envelope(n, false_fail_rate)
    return (high - low) * (sampling_envelope + analysis_envelope)


def min_num_samples_for_dkwm_mean_test(
    discrepancy, low, high,
    false_fail_rate=1e-6, false_pass_rate=1e-6, name=None):
  """Returns how many samples suffice for a one-sample DKWM mean test.

  To wit, returns an upper bound on the number of samples necessary to
  guarantee detecting a mean difference of at least the given
  `discrepancy`, with the given `false_fail_rate` and `false_pass_rate`,
  using the [Dvoretzky-Kiefer-Wolfowitz-Massart inequality]
  (https://en.wikipedia.org/wiki/CDF-based_nonparametric_confidence_interval)
  on a scalar distribution supported on `[low, high]`.

  Args:
    discrepancy: Floating-point tensor of desired upper limits on mean
      differences that may go undetected with probability higher than
      `1 - false_pass_rate`.
    low: Tensor of lower bounds on the distributions' support.
    high: Tensor of upper bounds on the distributions' support.
    false_fail_rate: *Scalar* admissible total rate of false failures.
    false_pass_rate: *Scalar* admissible rate of false passes.
    name: A name for this operation (optional).

  Returns:
    n: Tensor of numbers of samples to be drawn from the distributions
      of interest.

  The `discrepancy`, `low`, and `high` tensors must have
  broadcast-compatible shapes.

  For each batch member `i`, of `K` total, drawing `n[i]` samples from
  some scalar distribution supported on `[low[i], high[i]]` is enough
  to detect a difference in means of size `discrepancy[i]` or more.
  Specifically, we guarantee that (a) if the true mean is the expected
  mean, `assert_true_mean_equal_by_dkwm` will fail with probability at
  most `false_fail_rate / K` (which amounts to `false_fail_rate` if
  applied to the whole batch at once), and (b) if the true mean
  differs from the expected mean by at least `discrepancy[i]`,
  `assert_true_mean_equal_by_dkwm` will pass with probability at most
  `false_pass_rate`.

  The required number of samples scales
  as `O((high[i] - low[i])**2)`, `O(-log(false_fail_rate/K))`,
  `O(-log(false_pass_rate))`, and `O(1 / discrepancy[i]**2)`.
  """
  with ops.name_scope(
      name, "min_num_samples_for_dkwm_mean_test",
      [low, high, false_fail_rate, false_pass_rate, discrepancy]):
    discrepancy = ops.convert_to_tensor(
        discrepancy, name="discrepancy")
    low = ops.convert_to_tensor(low, name="low")
    high = ops.convert_to_tensor(high, name="high")
    false_fail_rate = ops.convert_to_tensor(
        false_fail_rate, name="false_fail_rate")
    false_pass_rate = ops.convert_to_tensor(
        false_pass_rate, name="false_pass_rate")
    # Could choose to cleverly allocate envelopes, but this is sound.
    envelope1 = discrepancy / (2. * (high - low))
    envelope2 = envelope1
    false_fail_rate = _itemwise_error_rate(
        false_fail_rate, [low, high, discrepancy])
    n1 = -math_ops.log(false_fail_rate / 2.) / (2. * envelope1**2)
    n2 = -math_ops.log(false_pass_rate / 2.) / (2. * envelope2**2)
    return math_ops.maximum(n1, n2)


def assert_true_mean_equal_by_dkwm_two_sample(
    samples1, low1, high1, samples2, low2, high2,
    false_fail_rate=1e-6, name=None):
  """Asserts the means of the given distributions are equal.

  More precisely, fails if there is enough evidence (using the
  [Dvoretzky-Kiefer-Wolfowitz-Massart inequality]
  (https://en.wikipedia.org/wiki/CDF-based_nonparametric_confidence_interval))
  that the means of the distributions from which the given samples are
  drawn are _not_ equal with statistical significance `false_fail_rate`
  or stronger, otherwise passes.  If you also want to check that you
  are gathering enough evidence that a pass is not spurious, see
  `min_num_samples_for_dkwm_mean_two_sample_test` and
  `min_discrepancy_of_true_means_detectable_by_dkwm_two_sample`.

  Note that `false_fail_rate` is a total false failure rate for all
  the assertions in the batch.  As such, if the batch is nontrivial,
  the assertion will insist on stronger evidence to fail any one member.

  Args:
    samples1: Floating-point tensor of samples from the
      distribution(s) A.  Entries are assumed IID across the 0th
      dimension.  The other dimensions must broadcast with `low1`,
      `high1`, `low2`, and `high2`.
    low1: Floating-point tensor of lower bounds on the supports of the
      distributions A.
    high1: Floating-point tensor of upper bounds on the supports of
      the distributions A.
    samples2: Floating-point tensor of samples from the
      distribution(s) B.  Entries are assumed IID across the 0th
      dimension.  The other dimensions must broadcast with `low1`,
      `high1`, `low2`, and `high2`.
    low2: Floating-point tensor of lower bounds on the supports of the
      distributions B.
    high2: Floating-point tensor of upper bounds on the supports of
      the distributions B.
    false_fail_rate: *Scalar* admissible total rate of mistakes.
    name: A name for this operation (optional).

  Returns:
    check: Op that raises `InvalidArgumentError` if any pair of confidence
      intervals true for corresponding true means do not overlap.
  """
  with ops.name_scope(
      name, "assert_true_mean_equal_by_dkwm_two_sample",
      [samples1, low1, high1, samples2, low2, high2, false_fail_rate]):
    samples1 = ops.convert_to_tensor(samples1, name="samples1")
    low1 = ops.convert_to_tensor(low1, name="low1")
    high1 = ops.convert_to_tensor(high1, name="high1")
    samples2 = ops.convert_to_tensor(samples2, name="samples2")
    low2 = ops.convert_to_tensor(low2, name="low2")
    high2 = ops.convert_to_tensor(high2, name="high2")
    false_fail_rate = ops.convert_to_tensor(
        false_fail_rate, name="false_fail_rate")
    samples1 = _check_shape_dominates(samples1, [low1, high1])
    samples2 = _check_shape_dominates(samples2, [low2, high2])
    compatible_samples = check_ops.assert_equal(
        array_ops.shape(samples1)[1:], array_ops.shape(samples2)[1:])
    with ops.control_dependencies([compatible_samples]):
      # Could in principle play games with cleverly allocating
      # significance instead of the even split below.  It may be possible
      # to get tighter intervals, in order to obtain a higher power test.
      # Any allocation strategy that depends only on the support bounds
      # and sample counts should be valid; however, because the intervals
      # scale as O(-log(false_fail_rate)), there doesn't seem to be much
      # room to win.
      min_mean_1, max_mean_1 = true_mean_confidence_interval_by_dkwm(
          samples1, low1, high1, false_fail_rate / 2.)
      min_mean_2, max_mean_2 = true_mean_confidence_interval_by_dkwm(
          samples2, low2, high2, false_fail_rate / 2.)
      # I want to assert
      #   not (max_mean_1 < min_mean_2 or min_mean_1 > max_mean_2),
      # but I think I only have and-combination of asserts, so use DeMorgan.
      clause1_op = check_ops.assert_greater_equal(max_mean_1, min_mean_2)
      with ops.control_dependencies([clause1_op]):
        return check_ops.assert_less_equal(min_mean_1, max_mean_2)


def min_discrepancy_of_true_means_detectable_by_dkwm_two_sample(
    n1, low1, high1, n2, low2, high2,
    false_fail_rate, false_pass_rate, name=None):
  """Returns the minimum mean discrepancy for a two-sample DKWM-based test.

  DKWM is the [Dvoretzky-Kiefer-Wolfowitz-Massart inequality]
  (https://en.wikipedia.org/wiki/CDF-based_nonparametric_confidence_interval).

  Note that `false_fail_rate` is a total false failure rate for all
  the tests in the batch.  As such, if the batch is nontrivial, each
  member will demand more samples.  The `false_pass_rate` is also
  interpreted as a total, but is treated asymmetrically: If each test
  in the batch detects its corresponding discrepancy with probability
  at least `1 - false_pass_rate`, then running all those tests and
  failing if any one fails will jointly detect all those discrepancies
  with the same `false_pass_rate`.

  Args:
    n1: Tensor of numbers of samples to be drawn from the distributions A.
    low1: Floating-point tensor of lower bounds on the supports of the
      distributions A.
    high1: Floating-point tensor of upper bounds on the supports of
      the distributions A.
    n2: Tensor of numbers of samples to be drawn from the distributions B.
    low2: Floating-point tensor of lower bounds on the supports of the
      distributions B.
    high2: Floating-point tensor of upper bounds on the supports of
      the distributions B.
    false_fail_rate: *Scalar* admissible total rate of false failures.
    false_pass_rate: *Scalar* admissible rate of false passes.
    name: A name for this operation (optional).

  Returns:
    discr: Tensor of lower bounds on the distances between true means
       detectable by a two-sample DKWM-based test.

  For each batch member `i`, of `K` total, drawing `n1[i]` samples
  from scalar distribution A supported on `[low1[i], high1[i]]` and `n2[i]`
  samples from scalar distribution B supported on `[low2[i], high2[i]]`
  is enough to detect a difference in their true means of size
  `discr[i]` or more.  Specifically, we guarantee that (a) if their
  true means are equal, `assert_true_mean_equal_by_dkwm_two_sample`
  will fail with probability at most `false_fail_rate/K` (which
  amounts to `false_fail_rate` if applied to the whole batch at once),
  and (b) if their true means differ by at least `discr[i]`,
  `assert_true_mean_equal_by_dkwm_two_sample` will pass with
  probability at most `false_pass_rate`.

  The detectable distribution scales as

  - `O(high1[i] - low1[i])`, `O(high2[i] - low2[i])`,
  - `O(1 / sqrt(n1[i]))`, `O(1 / sqrt(n2[i]))`,
  - `O(-log(false_fail_rate/K))`, and
  - `O(-log(false_pass_rate))`.
  """
  with ops.name_scope(
      name, "min_discrepancy_of_true_means_detectable_by_dkwm_two_sample",
      [n1, low1, high1, n2, low2, high2, false_fail_rate, false_pass_rate]):
    n1 = ops.convert_to_tensor(n1, name="n1")
    low1 = ops.convert_to_tensor(low1, name="low1")
    high1 = ops.convert_to_tensor(high1, name="high1")
    n2 = ops.convert_to_tensor(n2, name="n2")
    low2 = ops.convert_to_tensor(low2, name="low2")
    high2 = ops.convert_to_tensor(high2, name="high2")
    false_fail_rate = ops.convert_to_tensor(
        false_fail_rate, name="false_fail_rate")
    false_pass_rate = ops.convert_to_tensor(
        false_pass_rate, name="false_pass_rate")
    det_disc1 = min_discrepancy_of_true_means_detectable_by_dkwm(
        n1, low1, high1, false_fail_rate / 2., false_pass_rate / 2.)
    det_disc2 = min_discrepancy_of_true_means_detectable_by_dkwm(
        n2, low2, high2, false_fail_rate / 2., false_pass_rate / 2.)
    return det_disc1 + det_disc2


def min_num_samples_for_dkwm_mean_two_sample_test(
    discrepancy, low1, high1, low2, high2,
    false_fail_rate=1e-6, false_pass_rate=1e-6, name=None):
  """Returns how many samples suffice for a two-sample DKWM mean test.

  DKWM is the [Dvoretzky-Kiefer-Wolfowitz-Massart inequality]
  (https://en.wikipedia.org/wiki/CDF-based_nonparametric_confidence_interval).

  Args:
    discrepancy: Floating-point tensor of desired upper limits on mean
      differences that may go undetected with probability higher than
      `1 - false_pass_rate`.
    low1: Floating-point tensor of lower bounds on the supports of the
      distributions A.
    high1: Floating-point tensor of upper bounds on the supports of
      the distributions A.
    low2: Floating-point tensor of lower bounds on the supports of the
      distributions B.
    high2: Floating-point tensor of upper bounds on the supports of
      the distributions B.
    false_fail_rate: *Scalar* admissible total rate of false failures.
    false_pass_rate: *Scalar* admissible rate of false passes.
    name: A name for this operation (optional).

  Returns:
    n1: Tensor of numbers of samples to be drawn from the distributions A.
    n2: Tensor of numbers of samples to be drawn from the distributions B.

  For each batch member `i`, of `K` total, drawing `n1[i]` samples
  from scalar distribution A supported on `[low1[i], high1[i]]` and `n2[i]`
  samples from scalar distribution B supported on `[low2[i], high2[i]]`
  is enough to detect a difference in their true means of size
  `discr[i]` or more.  Specifically, we guarantee that (a) if their
  true means are equal, `assert_true_mean_equal_by_dkwm_two_sample`
  will fail with probability at most `false_fail_rate/K` (which
  amounts to `false_fail_rate` if applied to the whole batch at once),
  and (b) if their true means differ by at least `discr[i]`,
  `assert_true_mean_equal_by_dkwm_two_sample` will pass with
  probability at most `false_pass_rate`.

  The required number of samples scales as

  - `O((high1[i] - low1[i])**2)`, `O((high2[i] - low2[i])**2)`,
  - `O(-log(false_fail_rate/K))`,
  - `O(-log(false_pass_rate))`, and
  - `O(1 / discrepancy[i]**2)`.
  """
  with ops.name_scope(
      name, "min_num_samples_for_dkwm_mean_two_sample_test",
      [low1, high1, low2, high2,
       false_fail_rate, false_pass_rate, discrepancy]):
    discrepancy = ops.convert_to_tensor(discrepancy, name="discrepancy")
    low1 = ops.convert_to_tensor(low1, name="low1")
    high1 = ops.convert_to_tensor(high1, name="high1")
    low2 = ops.convert_to_tensor(low2, name="low2")
    high2 = ops.convert_to_tensor(high2, name="high2")
    false_fail_rate = ops.convert_to_tensor(
        false_fail_rate, name="false_fail_rate")
    false_pass_rate = ops.convert_to_tensor(
        false_pass_rate, name="false_pass_rate")
    # Could choose to cleverly allocate discrepancy tolerances and
    # failure probabilities, but this is sound.
    n1 = min_num_samples_for_dkwm_mean_test(
        discrepancy / 2., low1, high1,
        false_fail_rate / 2., false_pass_rate / 2.)
    n2 = min_num_samples_for_dkwm_mean_test(
        discrepancy / 2., low2, high2,
        false_fail_rate / 2., false_pass_rate / 2.)
    return n1, n2
