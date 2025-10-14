; ModuleID = 'summit'
source_filename = "summit"

%module_t = type opaque
%module_t.0 = type opaque

@lib = constant %module_t zeroinitializer
@out = constant %module_t.0 zeroinitializer
@0 = private unnamed_addr constant [5 x i8] c"true\00", align 1
@1 = private unnamed_addr constant [6 x i8] c"false\00", align 1

define i1 @__user_main() {
entry:
  %not_flag = alloca i1, align 1
  %flag = alloca i1, align 1
  store i1 true, ptr %flag, align 1
  %flag1 = load i1, ptr %flag, align 1
  %nottmp = xor i1 %flag1, true
  store i1 %nottmp, ptr %not_flag, align 1
  %not_flag2 = load i1, ptr %not_flag, align 1
  %str_buffer = alloca i8, i64 64, align 1
  %0 = select i1 %not_flag2, ptr @0, ptr @1
  call void @io_println_str(ptr %0)
  ret i1 false
}

declare void @io_println_str(ptr)

declare i32 @sprintf(ptr, ptr, ...)

define i32 @main() {
entry:
  %0 = call i1 @__user_main()
  %1 = zext i1 %0 to i32
  ret i32 %1
}
