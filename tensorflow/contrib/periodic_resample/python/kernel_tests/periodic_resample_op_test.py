# =============================================================================
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
# =============================================================================

import numpy
import tensorflow
from tensorflow.contrib.periodic_resample import periodic_resample


class PeriodicResampleTest(tensorflow.test.TestCase):
    def testPeriodicResample(self):

        # basic 2-D tensor
        input_tensor = numpy.arange(12).reshape((3, 4))
        desired_shape = numpy.array([6, -1])
        output_tensor = input_tensor.reshape((6, 2))
        with self.test_session():
            result = periodic_resample(input_tensor, desired_shape)
            self.assertAllEqual(result.eval(), output_tensor)

        # basic 2-D tensor (truncated)
        input_tensor = numpy.arange(12).reshape((3, 4))
        desired_shape = numpy.array([5, -1])
        output_tensor = input_tensor.reshape((6, 2))[:-1]
        with self.test_session():
            result = periodic_resample(input_tensor, desired_shape)
            self.assertAllEqual(result.eval(), output_tensor)


if __name__ == "__main__":
    tensorflow.test.main()
