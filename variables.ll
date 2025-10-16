; ModuleID = 'summit'
source_filename = "summit"

%module_t = type opaque
%module_t.0 = type opaque
%Person = type { ptr, i32, double }

@0 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@lib = constant %module_t zeroinitializer
@out = constant %module_t.0 zeroinitializer
@1 = private unnamed_addr constant [9 x i8] c"John Doe\00", align 1
@2 = private unnamed_addr constant [11 x i8] c"Mary Smith\00", align 1
@3 = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@4 = private unnamed_addr constant [3 x i8] c"%g\00", align 1

define ptr @Person.greet(ptr %self) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  ret ptr @0
}

define i1 @__user_main() {
entry:
  %mary = alloca %Person, align 8
  %john = alloca %Person, align 8
  %Person_tmp = alloca %Person, align 8
  %name = getelementptr inbounds %Person, ptr %Person_tmp, i32 0, i32 0
  store ptr @1, ptr %name, align 8
  %age = getelementptr inbounds %Person, ptr %Person_tmp, i32 0, i32 1
  store i64 30, ptr %age, align 4
  %height = getelementptr inbounds %Person, ptr %Person_tmp, i32 0, i32 2
  store double 1.750000e+00, ptr %height, align 8
  %Person_val = load %Person, ptr %Person_tmp, align 8
  store %Person %Person_val, ptr %john, align 8
  %Person_tmp1 = alloca %Person, align 8
  %name2 = getelementptr inbounds %Person, ptr %Person_tmp1, i32 0, i32 0
  store ptr @2, ptr %name2, align 8
  %age3 = getelementptr inbounds %Person, ptr %Person_tmp1, i32 0, i32 1
  store i64 25, ptr %age3, align 4
  %height4 = getelementptr inbounds %Person, ptr %Person_tmp1, i32 0, i32 2
  store double 1.650000e+00, ptr %height4, align 8
  %Person_val5 = load %Person, ptr %Person_tmp1, align 8
  store %Person %Person_val5, ptr %mary, align 8
  %name6 = getelementptr inbounds %Person, ptr %mary, i32 0, i32 0
  %name7 = load ptr, ptr %name6, align 8
  call void @io_println_str(ptr %name7)
  %age8 = getelementptr inbounds %Person, ptr %mary, i32 0, i32 1
  %age9 = load i32, ptr %age8, align 4
  %str_buffer = alloca i8, i64 64, align 1
  %0 = call i32 (ptr, ptr, ...) @sprintf(ptr %str_buffer, ptr @3, i32 %age9)
  call void @io_println_str(ptr %str_buffer)
  %height10 = getelementptr inbounds %Person, ptr %mary, i32 0, i32 2
  %height11 = load double, ptr %height10, align 8
  %str_buffer12 = alloca i8, i64 64, align 1
  %1 = call i32 (ptr, ptr, ...) @sprintf(ptr %str_buffer12, ptr @4, double %height11)
  call void @io_println_str(ptr %str_buffer12)
  %2 = call ptr @Person.greet(ptr %mary)
  call void @io_println_str(ptr %2)
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
