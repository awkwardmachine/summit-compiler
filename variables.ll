; ModuleID = 'summit'
source_filename = "summit"

%std_module_t = type opaque
%io_module_t = type opaque
%module_t = type opaque

@std = constant %std_module_t zeroinitializer
@io = constant %io_module_t zeroinitializer
@0 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@1 = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@2 = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@3 = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@4 = private unnamed_addr constant [5 x i8] c"%lld\00", align 1
@5 = private unnamed_addr constant [3 x i8] c"%f\00", align 1
@6 = private unnamed_addr constant [4 x i8] c"%lf\00", align 1
@7 = private unnamed_addr constant [5 x i8] c"true\00", align 1
@8 = private unnamed_addr constant [6 x i8] c"false\00", align 1
@lib = constant %module_t zeroinitializer
@9 = private unnamed_addr constant [14 x i8] c"Hello, world!\00", align 1
@10 = private unnamed_addr constant [8 x i8] c"Also hi\00", align 1

define void @io_println_str(ptr %str) {
entry:
  %0 = call i32 (ptr, ...) @printf(ptr @0, ptr %str)
  ret void
}

declare i32 @printf(ptr, ...)

declare i32 @sprintf(ptr, ptr, ...)

declare ptr @malloc(i64)

declare i32 @snprintf(ptr, i64, ptr, ...)

define ptr @int8_to_string(i8 %0) {
entry:
  %1 = call ptr @malloc(i64 32)
  %2 = sext i8 %0 to i32
  %3 = call i32 (ptr, i64, ptr, ...) @snprintf(ptr %1, i64 32, ptr @1, i32 %2)
  ret ptr %1
}

define ptr @int16_to_string(i16 %0) {
entry:
  %1 = call ptr @malloc(i64 32)
  %2 = sext i16 %0 to i32
  %3 = call i32 (ptr, i64, ptr, ...) @snprintf(ptr %1, i64 32, ptr @2, i32 %2)
  ret ptr %1
}

define ptr @int32_to_string(i32 %0) {
entry:
  %1 = call ptr @malloc(i64 32)
  %2 = call i32 (ptr, i64, ptr, ...) @snprintf(ptr %1, i64 32, ptr @3, i32 %0)
  ret ptr %1
}

define ptr @int64_to_string(i64 %0) {
entry:
  %1 = call ptr @malloc(i64 32)
  %2 = call i32 (ptr, i64, ptr, ...) @snprintf(ptr %1, i64 32, ptr @4, i64 %0)
  ret ptr %1
}

define ptr @float_to_string(float %0) {
entry:
  %1 = call ptr @malloc(i64 32)
  %2 = call i32 (ptr, i64, ptr, ...) @snprintf(ptr %1, i64 32, ptr @5, float %0)
  ret ptr %1
}

define ptr @double_to_string(double %0) {
entry:
  %1 = call ptr @malloc(i64 32)
  %2 = call i32 (ptr, i64, ptr, ...) @snprintf(ptr %1, i64 32, ptr @6, double %0)
  ret ptr %1
}

define ptr @bool_to_string(i1 %0) {
entry:
  br i1 %0, label %true, label %false

true:                                             ; preds = %entry
  %1 = call ptr @malloc(i64 6)
  %2 = call ptr @strcpy(ptr %1, ptr @7)
  br label %merge

false:                                            ; preds = %entry
  %3 = call ptr @malloc(i64 7)
  %4 = call ptr @strcpy(ptr %3, ptr @8)
  br label %merge

merge:                                            ; preds = %false, %true
  %result = phi ptr [ %1, %true ], [ %3, %false ]
  ret ptr %result
}

declare ptr @strcpy(ptr, ptr)

define i32 @main() {
entry:
  call void @io_println_str(ptr @9)
  call void @io_println_str(ptr @10)
  ret i32 0
}
