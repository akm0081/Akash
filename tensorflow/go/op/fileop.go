// Copyright 2017 The TensorFlow Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// DO NOT EDIT
// This file was machine generated by github.com/ctava/tensorflow/tensorflow/go/genop/wrap
//
// WARNING: This generation of wrapper function for TensorFlow ops is in an
// experimental state. The generated API can change without notice.

package op

import tf "github.com/tensorflow/tensorflow/tensorflow/go"

// Generate a glob pattern matching all sharded file names.
func ShardedFilespec(scope *Scope, basename tf.Output, num_shards tf.Output)(filename tf.Output) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "ShardedFilespec",
		Input: []tf.Input{
			basename, num_shards, 
		},
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// Writes contents to the file at input filename. Creates file if not existing.
//
// Arguments:
//	filename: scalar. The name of the file to which we write the contents.
//	contents: scalar. The content to be written to the output file.
//
// Returns the created operation.
func WriteFile(scope *Scope, filename tf.Output, contents tf.Output)(o *tf.Operation) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "WriteFile",
		Input: []tf.Input{
			filename, contents, 
		},
	}
	return scope.AddOperation(opspec)
}

// WholeFileReaderV2Attr is an optional argument to WholeFileReaderV2.
type WholeFileReaderV2Attr func(optionalAttr)


// WholeFileReaderV2Container sets the optional container attribute to value.
//
// value: If non-empty, this reader is placed in the given container.
// Otherwise, a default container is used.
// If not specified, defaults to "" 
func WholeFileReaderV2Container(value string) WholeFileReaderV2Attr {
	return func(m optionalAttr) {
		m["container"] = value
	}
}

// WholeFileReaderV2SharedName sets the optional shared_name attribute to value.
//
// value: If non-empty, this reader is named in the given bucket
// with this shared_name. Otherwise, the node name is used instead.
// If not specified, defaults to "" 
func WholeFileReaderV2SharedName(value string) WholeFileReaderV2Attr {
	return func(m optionalAttr) {
		m["shared_name"] = value
	}
}

// A Reader that outputs the entire contents of a file as a value.
//
// To use, enqueue filenames in a Queue.  The output of ReaderRead will
// be a filename (key) and the contents of that file (value).
//
// Returns The handle to reference the Reader.
func WholeFileReaderV2(scope *Scope, optional ...WholeFileReaderV2Attr)(reader_handle tf.Output) {
	if scope.Err() != nil {
		return
	}
	attrs := map[string]interface{}{}
	for _, a := range optional {
		a(attrs)
	}
	opspec := tf.OpSpec{
		Type: "WholeFileReaderV2",
		
		Attrs: attrs,
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// Returns the set of files matching one or more glob patterns.
//
// Note that this routine only supports wildcard characters in the
// basename portion of the pattern, not in the directory portion.
//
// Arguments:
//	pattern: Shell wildcard pattern(s). Scalar or vector of type string.
//
// Returns A vector of matching filenames.
func MatchingFiles(scope *Scope, pattern tf.Output)(filenames tf.Output) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "MatchingFiles",
		Input: []tf.Input{
			pattern, 
		},
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// Generate a sharded filename. The filename is printf formatted as
//
//    %s-%05d-of-%05d, basename, shard, num_shards.
func ShardedFilename(scope *Scope, basename tf.Output, shard tf.Output, num_shards tf.Output)(filename tf.Output) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "ShardedFilename",
		Input: []tf.Input{
			basename, shard, num_shards, 
		},
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// Reads and outputs the entire contents of the input filename.
func ReadFile(scope *Scope, filename tf.Output)(contents tf.Output) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "ReadFile",
		Input: []tf.Input{
			filename, 
		},
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// Generate a glob pattern matching all sharded file names.
func ShardedFilespec(scope *Scope, basename tf.Output, num_shards tf.Output)(filename tf.Output) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "ShardedFilespec",
		Input: []tf.Input{
			basename, num_shards, 
		},
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// Writes contents to the file at input filename. Creates file if not existing.
//
// Arguments:
//	filename: scalar. The name of the file to which we write the contents.
//	contents: scalar. The content to be written to the output file.
//
// Returns the created operation.
func WriteFile(scope *Scope, filename tf.Output, contents tf.Output)(o *tf.Operation) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "WriteFile",
		Input: []tf.Input{
			filename, contents, 
		},
	}
	return scope.AddOperation(opspec)
}

// WholeFileReaderV2Attr is an optional argument to WholeFileReaderV2.
type WholeFileReaderV2Attr func(optionalAttr)


// WholeFileReaderV2Container sets the optional container attribute to value.
//
// value: If non-empty, this reader is placed in the given container.
// Otherwise, a default container is used.
// If not specified, defaults to "" 
func WholeFileReaderV2Container(value string) WholeFileReaderV2Attr {
	return func(m optionalAttr) {
		m["container"] = value
	}
}

// WholeFileReaderV2SharedName sets the optional shared_name attribute to value.
//
// value: If non-empty, this reader is named in the given bucket
// with this shared_name. Otherwise, the node name is used instead.
// If not specified, defaults to "" 
func WholeFileReaderV2SharedName(value string) WholeFileReaderV2Attr {
	return func(m optionalAttr) {
		m["shared_name"] = value
	}
}

// A Reader that outputs the entire contents of a file as a value.
//
// To use, enqueue filenames in a Queue.  The output of ReaderRead will
// be a filename (key) and the contents of that file (value).
//
// Returns The handle to reference the Reader.
func WholeFileReaderV2(scope *Scope, optional ...WholeFileReaderV2Attr)(reader_handle tf.Output) {
	if scope.Err() != nil {
		return
	}
	attrs := map[string]interface{}{}
	for _, a := range optional {
		a(attrs)
	}
	opspec := tf.OpSpec{
		Type: "WholeFileReaderV2",
		
		Attrs: attrs,
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// Returns the set of files matching one or more glob patterns.
//
// Note that this routine only supports wildcard characters in the
// basename portion of the pattern, not in the directory portion.
//
// Arguments:
//	pattern: Shell wildcard pattern(s). Scalar or vector of type string.
//
// Returns A vector of matching filenames.
func MatchingFiles(scope *Scope, pattern tf.Output)(filenames tf.Output) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "MatchingFiles",
		Input: []tf.Input{
			pattern, 
		},
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// Generate a sharded filename. The filename is printf formatted as
//
//    %s-%05d-of-%05d, basename, shard, num_shards.
func ShardedFilename(scope *Scope, basename tf.Output, shard tf.Output, num_shards tf.Output)(filename tf.Output) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "ShardedFilename",
		Input: []tf.Input{
			basename, shard, num_shards, 
		},
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// Reads and outputs the entire contents of the input filename.
func ReadFile(scope *Scope, filename tf.Output)(contents tf.Output) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "ReadFile",
		Input: []tf.Input{
			filename, 
		},
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}
