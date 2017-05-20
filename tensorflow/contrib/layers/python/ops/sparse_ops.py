# Copyright 2016 The TensorFlow Authors. All Rights Reserved.
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
"""Ops to work with `SparseTensor`."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from tensorflow.python.framework import dtypes
from tensorflow.python.framework import ops
from tensorflow.python.framework import sparse_tensor
from tensorflow.python.ops import array_ops
from tensorflow.python.ops import math_ops
from tensorflow.python.util import compat


def _multiplier_helper(shape):
  """Returns moving offset for each dimension given shape."""
  multipliers = []
  for dim in reversed(shape):
    if multipliers:
      multipliers.append(dim * multipliers[-1])
    else:
      multipliers.append(dim)
  multipliers.reverse()
  return multipliers


def _ignore_value(dtype):
  if dtype == dtypes.string:
    # Exception due to TF strings are converted to numpy objects by default.
    return ""
  # NOTE: `as_numpy_dtype` is a property, so with the parentheses this is
  # constructing a new numpy object of the given type, which yields the
  # default value for that type.
  return dtype.as_numpy_dtype()


def dense_to_sparse_tensor(dense_tensor, ignore_value=None):
  """Converts dense `Tensor` to `SparseTensor`, dropping `ignore_value` cells.

  Args:
    dense_tensor: A `Tensor`. This must have a statically defined rank.
    ignore_value: Entries in `dense_tensor` equal to this value will be
      absent from the return `SparseTensor`. If `None`, default value of
      `dense_tensor` dtype will be used (e.g. '' for `str`, 0 for `int`).

  Returns:
    A `SparseTensor` with the same shape as `dense_tensor`.

  Raises:
    ValueError: when `dense_tensor`'s rank is `None`.
  """
  with ops.name_scope("DenseToSparseTensor"):
    dense_t = ops.convert_to_tensor(dense_tensor)
    if dense_t.get_shape().ndims is None:
      # TODO(b/32318825): Implement dense_to_sparse_tensor for undefined rank.
      raise ValueError("dense_tensor.get_shape() should be defined, got None.")
    if ignore_value is None:
      ignore_value = _ignore_value(dense_t.dtype)
    dense_shape = math_ops.cast(array_ops.shape(dense_t), dtypes.int64)
    indices = array_ops.where(
        math_ops.not_equal(dense_t, math_ops.cast(ignore_value, dense_t.dtype)))
    index_dims = len(dense_t.get_shape())
    # Flattens the tensor and indices for use with gather.
    flat_tensor = array_ops.reshape(dense_t, [-1])
    flat_indices = indices[:, index_dims - 1]
    # Computes the correct flattened indices for 2d (or higher) tensors.
    if index_dims > 1:
      higher_dims = indices[:, :index_dims - 1]
      shape_multipliers = array_ops.stack(
          _multiplier_helper(array_ops.unstack(dense_shape)[1:]))
      offsets = math_ops.reduce_sum(
          math_ops.multiply(higher_dims, shape_multipliers),
          reduction_indices=[1])
      flat_indices = math_ops.add(flat_indices, offsets)
    values = array_ops.gather(flat_tensor, flat_indices)
    return sparse_tensor.SparseTensor(indices, values, dense_shape)


# TODO(ptucker): Support integer dtype arg, and cast values back to that.
def indicators_to_sparse_ids(indicators, ignore_value=None, dtype=dtypes.int64):
  """Convert a dense indicator tensor to sparse IDs.

  This is commonly used for converting a dense classification label to sparse.
  In the following example, we have an input of shape (2, 2, num_classes),
  where num_classes=4.

  indicators = [
    [[0, 0, 1, 0], [0, 0, 0, 0]],
    [[1, 0, 1, 1], [0, 0, 1, 0]],
  ]
  indicator_to_sparse_ids(indicators) => [
    [[2], []],
    [[0, 2, 3], [2]],
  ]

  Args:
    indicators: Dense `Tensor` of shape `(d0, ..., dn, num_classes)`. This must
      have a statically defined rank. `ignore_value` values are ignored. For
      other values (typically, ones), the index along the last dimension is
      returned.
    ignore_value: Entries in `indicators` equal to this value will be
      absent from the returned `SparseTensor`. If `None`, default value of
      `indicators` dtype will be used (e.g. '' for `str`, 0 for `int`).
    dtype: Type of result, must be integer type.

  Returns:
    `tf.int64` `SparseTensor` of shape `(d0, ..., dn, max_num_labels)`,
      where `max_num_labels` is the maximum number of non-zero values in any
      row (in the example above, row (1, 1) has 3 non-zero values, so the result
      shape is (2, 2, 3)). The values of this `SparseTensor` are in the range
      `[0, num_classes)` and correspond to the index of non-empty values along
      the last dimension of `indicators`.

  Raises:
    ValueError: if `dtype` is not integer.
  """
  if not dtype.is_integer:
    raise ValueError("Invalid dtype {} not integer.".format(dtype))
  with ops.name_scope(
      None, "indicators_to_sparse_ids", (indicators, ignore_value)):
    # Convert indicators to binary ones and zeros. We use int64 since
    # SparseTensor requires int64 indices.
    indicators = ops.convert_to_tensor(indicators, name="indicators")
    if ignore_value is None:
      ignore_value = _ignore_value(indicators.dtype)
    missing_indicators = math_ops.equal(
        indicators, ignore_value, name="missing")
    zeros_like_indicators = array_ops.zeros_like(
        indicators, dtype=dtypes.int64, name="zeros")
    binary_indicators = array_ops.where(
        missing_indicators, zeros_like_indicators,
        array_ops.ones_like(indicators, dtype=dtypes.int64, name="ones"),
        name="binary_indicators")

    # Use cumsum along the last dimension to generate per-row indexes.
    # Note that these are 1-based (since 0 indicates missing values), so they're
    # off-by-1 from the actual indices. We'll subtract 1 below. Since they're
    # off-by-one, the max value is the size of last dimension (i.e.,
    # last_index + 1).
    row_index_indicators = array_ops.where(
        missing_indicators, zeros_like_indicators,
        math_ops.cumsum(binary_indicators, axis=-1), "row_index_indicators")
    result_last_dim = array_ops.reshape(
        math_ops.reduce_max(row_index_indicators), shape=(1,),
        name="result_last_dim")

    # Convert to a SparseTensor. The values of this SparseTensor are the last
    # indices of our result, and the last indices of this SparseTensor (i.e.,
    # the class IDs indicated by `indicators`) are the values of our result, so
    # we use unstack/stack to swap them.
    sparse_row_index_indicators = dense_to_sparse_tensor(
        row_index_indicators, ignore_value=0)
    index_columns = array_ops.unstack(
        sparse_row_index_indicators.indices, axis=1)
    return sparse_tensor.SparseTensor(
        indices=array_ops.stack(
            index_columns[0:-1] + [sparse_row_index_indicators.values - 1],
            axis=1, name="indices"),
        values=math_ops.cast(index_columns[-1], dtype=dtype, name="values"),
        dense_shape=array_ops.concat(
            (sparse_row_index_indicators.dense_shape[0:-1], result_last_dim),
            axis=0, name="dense_shape"))


def sparse_row_envelope(sparse_input, row_axis=0, col_axis=1, name=None):
  """Returns the length of each 'row' in a `SparseTensor`.

  For example, if `sparse_input` has indices `[[0,0], [2, 0], [2, 1], [2, 2]]`
  and shape `[3, 3]`, this function will return `[1, 0, 3]`.

  Args:
    sparse_input: a `SparseTensor` of rank at least 2.
    row_axis: An integer. The axis for the row of the envelope matrix. Default
      is 0.
    col_axis: An integer. The axis for the col of the envelope matrix. Default
      is 1.
    name: A name for the operation (optional).

  Returns:
    A one-dimensional `Tensor` whose entries correspond to the length of each
    row of `SparseTensor`.

  Raises:
    ValueError: If row_axis and col_axis are the same axis or they are not
      integers.
  """
  if not (isinstance(row_axis, compat.integral_types) and
          isinstance(col_axis, compat.integral_types)):
    raise ValueError("`row_axis` and `col_axis` must be integers.")

  if row_axis == col_axis:
    raise ValueError("Row and column can not be the same axis.")

  with ops.name_scope(name, "sparse_row_envelope", [sparse_input]):
    indices = sparse_input.indices
    row_indices = indices[:, row_axis]
    col_indices = indices[:, col_axis]
    num_rows = math_ops.cast(sparse_input.dense_shape[row_axis], dtypes.int32)
    row_envelope = math_ops.unsorted_segment_max(
        col_indices + 1, row_indices, num_rows, name=name)
    zeros = array_ops.zeros_like(row_envelope)
    return array_ops.where(row_envelope > zeros, row_envelope, zeros)
