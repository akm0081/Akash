# Copyright 2015 The TensorFlow Authors. All Rights Reserved.
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

"""Functional test for learning rate decay."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import math

from tensorflow.python.eager import context
from tensorflow.python.keras import combinations
from tensorflow.python.keras import keras_parameterized
from tensorflow.python.keras.optimizer_v2 import legacy_learning_rate_decay as learning_rate_decay
from tensorflow.python.ops import variables
from tensorflow.python.platform import googletest


@combinations.generate(combinations.combine(mode=["graph", "eager"]))
class LRDecayTest(keras_parameterized.TestCase):

  def testContinuous(self):
    self.evaluate(variables.global_variables_initializer())
    step = 5
    decayed_lr = learning_rate_decay.exponential_decay(0.05, step, 10, 0.96)
    expected = .05 * 0.96**(5.0 / 10.0)
    self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)

  def testStaircase(self):
    if context.executing_eagerly():
      step = variables.Variable(0)
      self.evaluate(variables.global_variables_initializer())
      decayed_lr = learning_rate_decay.exponential_decay(
          .1, step, 3, 0.96, staircase=True)

      # No change to learning rate due to staircase
      expected = .1
      self.evaluate(step.assign(1))
      self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)

      expected = .1
      self.evaluate(step.assign(2))
      self.assertAllClose(self.evaluate(decayed_lr), .1, 1e-6)

      # Decayed learning rate
      expected = .1 * 0.96 ** (100 // 3)
      self.evaluate(step.assign(100))
      self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)

  def testVariables(self):
    step = variables.VariableV1(1)

    decayed_lr = learning_rate_decay.exponential_decay(
        .1, step, 3, 0.96, staircase=True)
    self.evaluate(variables.global_variables_initializer())
    # No change to learning rate
    assign_1 = step.assign(1)
    if not context.executing_eagerly():
      self.evaluate(assign_1.op)
    self.assertAllClose(self.evaluate(decayed_lr), .1, 1e-6)
    assign_2 = step.assign(2)
    if not context.executing_eagerly():
      self.evaluate(assign_2.op)
    self.assertAllClose(self.evaluate(decayed_lr), .1, 1e-6)
    # Decayed learning rate
    assign_100 = step.assign(100)
    if not context.executing_eagerly():
      self.evaluate(assign_100.op)
    expected = .1 * 0.96**(100 // 3)
    self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)

  def testPiecewiseConstant(self):
    x = variables.Variable(-999)
    decayed_lr = learning_rate_decay.piecewise_constant(
        x, [100, 110, 120], [1.0, 0.1, 0.01, 0.001])

    self.evaluate(variables.global_variables_initializer())

    self.assertAllClose(self.evaluate(decayed_lr), 1.0, 1e-6)
    self.evaluate(x.assign(100))
    self.assertAllClose(self.evaluate(decayed_lr), 1.0, 1e-6)
    self.evaluate(x.assign(105))
    self.assertAllClose(self.evaluate(decayed_lr), 0.1, 1e-6)
    self.evaluate(x.assign(110))
    self.assertAllClose(self.evaluate(decayed_lr), 0.1, 1e-6)
    self.evaluate(x.assign(120))
    self.assertAllClose(self.evaluate(decayed_lr), 0.01, 1e-6)
    self.evaluate(x.assign(999))
    self.assertAllClose(self.evaluate(decayed_lr), 0.001, 1e-6)

  def testPiecewiseConstantEdgeCases(self):
    x_int = variables.Variable(0, dtype=variables.dtypes.int32)
    boundaries, values = [-1.0, 1.0], [1, 2, 3]
    with self.assertRaises(ValueError):
      decayed_lr = learning_rate_decay.piecewise_constant(
          x_int, boundaries, values)
      if context.executing_eagerly():
        decayed_lr()

    x = variables.Variable(0.0)
    boundaries, values = [-1.0, 1.0], [1.0, 2, 3]
    with self.assertRaises(ValueError):
      decayed_lr = learning_rate_decay.piecewise_constant(
          x, boundaries, values)
      if context.executing_eagerly():
        decayed_lr()

    # Test that ref types are valid.
    if not context.executing_eagerly():
      x = variables.VariableV1(0.0, use_resource=False)
      x_ref = x.op.outputs[0]   # float32_ref tensor should be accepted
      boundaries, values = [1.0, 2.0], [1, 2, 3]
      learning_rate_decay.piecewise_constant(x_ref, boundaries, values)

    # Test casting boundaries from int32 to int64.
    x_int64 = variables.Variable(0, dtype=variables.dtypes.int64)
    boundaries, values = [1, 2, 3], [0.4, 0.5, 0.6, 0.7]
    decayed_lr = learning_rate_decay.piecewise_constant(
        x_int64, boundaries, values)

    self.evaluate(variables.global_variables_initializer())
    self.assertAllClose(self.evaluate(decayed_lr), 0.4, 1e-6)
    self.evaluate(x_int64.assign(1))
    self.assertAllClose(self.evaluate(decayed_lr), 0.4, 1e-6)
    self.evaluate(x_int64.assign(2))
    self.assertAllClose(self.evaluate(decayed_lr), 0.5, 1e-6)
    self.evaluate(x_int64.assign(3))
    self.assertAllClose(self.evaluate(decayed_lr), 0.6, 1e-6)
    self.evaluate(x_int64.assign(4))
    self.assertAllClose(self.evaluate(decayed_lr), 0.7, 1e-6)


@combinations.generate(combinations.combine(mode=["graph", "eager"]))
class LinearDecayTest(keras_parameterized.TestCase):

  def testHalfWay(self):
    step = 5
    lr = 0.05
    end_lr = 0.0
    decayed_lr = learning_rate_decay.polynomial_decay(lr, step, 10, end_lr)
    expected = lr * 0.5
    self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)

  def testEnd(self):
    step = 10
    lr = 0.05
    end_lr = 0.001
    decayed_lr = learning_rate_decay.polynomial_decay(lr, step, 10, end_lr)
    expected = end_lr
    self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)

  def testHalfWayWithEnd(self):
    step = 5
    lr = 0.05
    end_lr = 0.001
    decayed_lr = learning_rate_decay.polynomial_decay(lr, step, 10, end_lr)
    expected = (lr + end_lr) * 0.5
    self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)

  def testBeyondEnd(self):
    step = 15
    lr = 0.05
    end_lr = 0.001
    decayed_lr = learning_rate_decay.polynomial_decay(lr, step, 10, end_lr)
    expected = end_lr
    self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)

  def testBeyondEndWithCycle(self):
    step = 15
    lr = 0.05
    end_lr = 0.001
    decayed_lr = learning_rate_decay.polynomial_decay(
        lr, step, 10, end_lr, cycle=True)
    expected = (lr - end_lr) * 0.25 + end_lr
    self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)


@combinations.generate(combinations.combine(mode=["graph", "eager"]))
class SqrtDecayTest(keras_parameterized.TestCase):

  def testHalfWay(self):
    step = 5
    lr = 0.05
    end_lr = 0.0
    power = 0.5
    decayed_lr = learning_rate_decay.polynomial_decay(
        lr, step, 10, end_lr, power=power)
    expected = lr * 0.5**power
    self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)

  def testEnd(self):
    step = 10
    lr = 0.05
    end_lr = 0.001
    power = 0.5
    decayed_lr = learning_rate_decay.polynomial_decay(
        lr, step, 10, end_lr, power=power)
    expected = end_lr
    self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)

  def testHalfWayWithEnd(self):
    step = 5
    lr = 0.05
    end_lr = 0.001
    power = 0.5
    decayed_lr = learning_rate_decay.polynomial_decay(
        lr, step, 10, end_lr, power=power)
    expected = (lr - end_lr) * 0.5**power + end_lr
    self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)

  def testBeyondEnd(self):
    step = 15
    lr = 0.05
    end_lr = 0.001
    power = 0.5
    decayed_lr = learning_rate_decay.polynomial_decay(
        lr, step, 10, end_lr, power=power)
    expected = end_lr
    self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)

  def testBeyondEndWithCycle(self):
    step = 15
    lr = 0.05
    end_lr = 0.001
    power = 0.5
    decayed_lr = learning_rate_decay.polynomial_decay(
        lr, step, 10, end_lr, power=power, cycle=True)
    expected = (lr - end_lr) * 0.25**power + end_lr
    self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)


@combinations.generate(combinations.combine(mode=["graph", "eager"]))
class PolynomialDecayTest(keras_parameterized.TestCase):

  def testBeginWithCycle(self):
    lr = 0.001
    decay_steps = 10
    step = 0
    decayed_lr = learning_rate_decay.polynomial_decay(
        lr, step, decay_steps, cycle=True)
    expected = lr
    self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)


@combinations.generate(combinations.combine(mode=["graph", "eager"]))
class ExponentialDecayTest(keras_parameterized.TestCase):

  def testDecay(self):
    initial_lr = 0.1
    k = 10
    decay_rate = 0.96
    step = variables.Variable(0)
    decayed_lr = learning_rate_decay.natural_exp_decay(initial_lr, step, k,
                                                       decay_rate)

    self.evaluate(variables.global_variables_initializer())
    for i in range(k + 1):
      expected = initial_lr * math.exp(-i / k * decay_rate)
      self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)
      self.evaluate(step.assign_add(1))

  def testStaircase(self):
    initial_lr = 0.1
    k = 10
    decay_rate = 0.96
    step = variables.Variable(0)
    decayed_lr = learning_rate_decay.natural_exp_decay(
        initial_lr, step, k, decay_rate, staircase=True)

    self.evaluate(variables.global_variables_initializer())
    for i in range(k + 1):
      expected = initial_lr * math.exp(-decay_rate * (i // k))
      self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)
      self.evaluate(step.assign_add(1))


@combinations.generate(combinations.combine(mode=["graph", "eager"]))
class InverseDecayTest(keras_parameterized.TestCase):

  def testDecay(self):
    initial_lr = 0.1
    k = 10
    decay_rate = 0.96
    step = variables.Variable(0)
    decayed_lr = learning_rate_decay.inverse_time_decay(initial_lr, step, k,
                                                        decay_rate)

    self.evaluate(variables.global_variables_initializer())
    for i in range(k + 1):
      expected = initial_lr / (1 + i / k * decay_rate)
      self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)
      self.evaluate(step.assign_add(1))

  def testStaircase(self):
    initial_lr = 0.1
    k = 10
    decay_rate = 0.96
    step = variables.Variable(0)
    decayed_lr = learning_rate_decay.inverse_time_decay(
        initial_lr, step, k, decay_rate, staircase=True)

    self.evaluate(variables.global_variables_initializer())
    for i in range(k + 1):
      expected = initial_lr / (1 + decay_rate * (i // k))
      self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)
      self.evaluate(step.assign_add(1))


@combinations.generate(combinations.combine(mode=["graph", "eager"]))
class CosineDecayTest(keras_parameterized.TestCase):

  def np_cosine_decay(self, step, decay_steps, alpha=0.0):
    step = min(step, decay_steps)
    completed_fraction = step / decay_steps
    decay = 0.5 * (1.0 + math.cos(math.pi * completed_fraction))
    return (1.0 - alpha) * decay + alpha

  def testDecay(self):
    num_training_steps = 1000
    initial_lr = 1.0
    for step in range(0, 1500, 250):
      decayed_lr = learning_rate_decay.cosine_decay(initial_lr, step,
                                                    num_training_steps)
      expected = self.np_cosine_decay(step, num_training_steps)
      self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)

  def testAlpha(self):
    num_training_steps = 1000
    initial_lr = 1.0
    alpha = 0.1
    for step in range(0, 1500, 250):
      decayed_lr = learning_rate_decay.cosine_decay(initial_lr, step,
                                                    num_training_steps, alpha)
      expected = self.np_cosine_decay(step, num_training_steps, alpha)
      self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)


@combinations.generate(combinations.combine(mode=["graph", "eager"]))
class CosineDecayRestartsTest(keras_parameterized.TestCase):

  def np_cosine_decay_restarts(self, step, decay_steps, t_mul=2.0, m_mul=1.0,
                               alpha=0.0):
    fac = 1.0
    while step >= decay_steps:
      step -= decay_steps
      decay_steps *= t_mul
      fac *= m_mul

    completed_fraction = step / decay_steps
    decay = fac * 0.5 * (1.0 + math.cos(math.pi * completed_fraction))
    return (1.0 - alpha) * decay + alpha

  def testDecay(self):
    num_training_steps = 1000
    initial_lr = 1.0
    for step in range(0, 1500, 250):
      decayed_lr = learning_rate_decay.cosine_decay_restarts(
          initial_lr, step, num_training_steps)
      expected = self.np_cosine_decay_restarts(step, num_training_steps)
      self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)

  def testAlpha(self):
    num_training_steps = 1000
    initial_lr = 1.0
    alpha = 0.1
    for step in range(0, 1500, 250):
      decayed_lr = learning_rate_decay.cosine_decay_restarts(
          initial_lr, step, num_training_steps, alpha=alpha)
      expected = self.np_cosine_decay_restarts(
          step, num_training_steps, alpha=alpha)
      self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)

  def testMMul(self):
    num_training_steps = 1000
    initial_lr = 1.0
    m_mul = 0.9
    for step in range(0, 1500, 250):
      decayed_lr = learning_rate_decay.cosine_decay_restarts(
          initial_lr, step, num_training_steps, m_mul=m_mul)
      expected = self.np_cosine_decay_restarts(
          step, num_training_steps, m_mul=m_mul)
      self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)

  def testTMul(self):
    num_training_steps = 1000
    initial_lr = 1.0
    t_mul = 1.0
    for step in range(0, 1500, 250):
      decayed_lr = learning_rate_decay.cosine_decay_restarts(
          initial_lr, step, num_training_steps, t_mul=t_mul)
      expected = self.np_cosine_decay_restarts(
          step, num_training_steps, t_mul=t_mul)
      self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)


@combinations.generate(combinations.combine(mode=["graph", "eager"]))
class LinearCosineDecayTest(keras_parameterized.TestCase):

  def np_linear_cosine_decay(self,
                             step,
                             decay_steps,
                             alpha=0.0,
                             beta=0.001,
                             num_periods=0.5):
    step = min(step, decay_steps)
    linear_decayed = float(decay_steps - step) / decay_steps
    fraction = 2.0 * num_periods * step / float(decay_steps)
    cosine_decayed = 0.5 * (1.0 + math.cos(math.pi * fraction))
    return (alpha + linear_decayed) * cosine_decayed + beta

  def testDefaultDecay(self):
    num_training_steps = 1000
    initial_lr = 1.0
    for step in range(0, 1500, 250):
      decayed_lr = learning_rate_decay.linear_cosine_decay(
          initial_lr, step, num_training_steps)
      expected = self.np_linear_cosine_decay(step, num_training_steps)
      self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)

  def testNonDefaultDecay(self):
    num_training_steps = 1000
    initial_lr = 1.0
    for step in range(0, 1500, 250):
      decayed_lr = learning_rate_decay.linear_cosine_decay(
          initial_lr,
          step,
          num_training_steps,
          alpha=0.1,
          beta=1e-4,
          num_periods=5)
      expected = self.np_linear_cosine_decay(
          step, num_training_steps, alpha=0.1, beta=1e-4, num_periods=5)
      self.assertAllClose(self.evaluate(decayed_lr), expected, 1e-6)


@combinations.generate(combinations.combine(mode=["graph", "eager"]))
class NoisyLinearCosineDecayTest(keras_parameterized.TestCase):

  def testDefaultNoisyLinearCosine(self):
    num_training_steps = 1000
    initial_lr = 1.0
    for step in range(0, 1500, 250):
      # No numerical check because of noise
      decayed_lr = learning_rate_decay.noisy_linear_cosine_decay(
          initial_lr, step, num_training_steps)
      # Cannot be deterministically tested
      self.evaluate(decayed_lr)

  def testNonDefaultNoisyLinearCosine(self):
    num_training_steps = 1000
    initial_lr = 1.0
    for step in range(0, 1500, 250):
      # No numerical check because of noise
      decayed_lr = learning_rate_decay.noisy_linear_cosine_decay(
          initial_lr,
          step,
          num_training_steps,
          initial_variance=0.5,
          variance_decay=0.1,
          alpha=0.1,
          beta=1e-4,
          num_periods=5)
      # Cannot be deterministically tested
      self.evaluate(decayed_lr)


if __name__ == "__main__":
  googletest.main()
