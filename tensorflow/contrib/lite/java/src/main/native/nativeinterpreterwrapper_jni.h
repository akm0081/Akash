/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_CONTRIB_LITE_JAVA_NATIVEINTERPRETERWRAPPER_JNI_H_
#define TENSORFLOW_CONTRIB_LITE_JAVA_NATIVEINTERPRETERWRAPPER_JNI_H_

#include <jni.h>
#include <stdio.h>
#include <vector>
#include "tensorflow/contrib/lite/context.h"
#include "tensorflow/contrib/lite/interpreter.h"
#include "tensorflow/contrib/lite/java/src/main/native/exception_jni.h"
#include "tensorflow/contrib/lite/java/src/main/native/tensor_jni.h"
#include "tensorflow/contrib/lite/model.h"

namespace tflite {
// This is to be provided at link-time by a library.
extern std::unique_ptr<OpResolver> CreateOpResolver();
}  // namespace tflite

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/*
 *  Class:     org_tensorflow_lite_NativeInterpreterWrapper
 *  Method:
 *  Signature: (J)[Ljava/lang/Object;
 */
JNIEXPORT jobjectArray JNICALL
Java_org_tensorflow_lite_NativeInterpreterWrapper_getInputNames(JNIEnv* env,
                                                                jclass clazz,
                                                                jlong handle);

/*
 *  Class:     org_tensorflow_lite_NativeInterpreterWrapper
 *  Method:
 *  Signature: (J)[Ljava/lang/Object;
 */
JNIEXPORT jobjectArray JNICALL
Java_org_tensorflow_lite_NativeInterpreterWrapper_getOutputNames(JNIEnv* env,
                                                                 jclass clazz,
                                                                 jlong handle);

/*
 *  Class:     org_tensorflow_lite_NativeInterpreterWrapper
 *  Method:
 *  Signature: (JZ)
 */
JNIEXPORT void JNICALL
Java_org_tensorflow_lite_NativeInterpreterWrapper_useNNAPI(JNIEnv* env,
                                                           jclass clazz,
                                                           jlong handle,
                                                           jboolean state);

/*
 *  Class:     org_tensorflow_lite_NativeInterpreterWrapper
 *  Method:
 *  Signature: (I)J
 */
JNIEXPORT jlong JNICALL
Java_org_tensorflow_lite_NativeInterpreterWrapper_createErrorReporter(
    JNIEnv* env, jclass clazz, jint size);

/*
 *  Class:     org_tensorflow_lite_NativeInterpreterWrapper
 *  Method:
 *  Signature: (Ljava/lang/String;J)J
 */
JNIEXPORT jlong JNICALL
Java_org_tensorflow_lite_NativeInterpreterWrapper_createModel(
    JNIEnv* env, jclass clazz, jstring model_file, jlong error_handle);

/*
 *  Class:     org_tensorflow_lite_NativeInterpreterWrapper
 *  Method:
 *  Signature: (Ljava/lang/Object;J)J
 */
JNIEXPORT jlong JNICALL
Java_org_tensorflow_lite_NativeInterpreterWrapper_createModelWithBuffer(
    JNIEnv* env, jclass clazz, jobject model_buffer, jlong error_handle);

/*
 *  Class:     org_tensorflow_lite_NativeInterpreterWrapper
 *  Method:
 *  Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_org_tensorflow_lite_NativeInterpreterWrapper_createInterpreter(
    JNIEnv* env, jclass clazz, jlong model_handle);

/*
 *  Class:     org_tensorflow_lite_NativeInterpreterWrapper
 *  Method:
 *  Signature: (JJ[Ljava/lang/Object;[I[I[Ljava/lang/Object;)[J
 */
JNIEXPORT jlongArray JNICALL
Java_org_tensorflow_lite_NativeInterpreterWrapper_run(
    JNIEnv* env, jclass clazz, jlong interpreter_handle, jlong error_handle,
    jobjectArray sizes, jintArray data_types, jintArray nums_of_bytes,
    jobjectArray values);

/*
 *  Class:     org_tensorflow_lite_NativeInterpreterWrapper
 *  Method:
 *  Signature: (JII)[I
 *
 * It gets input dimensions if num_bytes matches number of bytes required by
 * the input, else returns null and throws IllegalArgumentException.
 */
JNIEXPORT jintArray JNICALL
Java_org_tensorflow_lite_NativeInterpreterWrapper_getInputDims(
    JNIEnv* env, jclass clazz, jlong handle, jint input_idx, jint num_bytes);

/*
 *  Class:     org_tensorflow_lite_NativeInterpreterWrapper
 *  Method:
 *  Signature: (JJI[I)
 *
 * It resizes dimensions of a input.
 */
JNIEXPORT void JNICALL
Java_org_tensorflow_lite_NativeInterpreterWrapper_resizeInput(
    JNIEnv* env, jclass clazz, jlong interpreter_handle, jlong error_handle,
    jint input_idx, jintArray dims);

/*
 *  Class:     org_tensorflow_lite_NativeInterpreterWrapper
 *  Method:
 *  Signature: (JJJ)
 */
JNIEXPORT void JNICALL Java_org_tensorflow_lite_NativeInterpreterWrapper_delete(
    JNIEnv* env, jclass clazz, jlong error_handle, jlong model_handle,
    jlong interpreter_handle);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus
#endif  // TENSORFLOW_CONTRIB_LITE_JAVA_NATIVEINTERPRETERWRAPPER_JNI_H_
