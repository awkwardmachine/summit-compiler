; ModuleID = 'summit'
source_filename = "summit"

%module_t = type opaque
%module_t.0 = type opaque
%module_t.1 = type opaque

@lib = constant %module_t zeroinitializer
@out = constant %module_t.0 zeroinitializer
@math = constant %module_t.1 zeroinitializer
@0 = private unnamed_addr constant [11 x i8] c"Calculator\00", align 1
@1 = private unnamed_addr constant [7 x i8] c"1: Add\00", align 1
@2 = private unnamed_addr constant [12 x i8] c"2: Subtract\00", align 1
@3 = private unnamed_addr constant [12 x i8] c"3: Multiply\00", align 1
@4 = private unnamed_addr constant [10 x i8] c"4: Divide\00", align 1
@5 = private unnamed_addr constant [8 x i8] c"0: Exit\00", align 1
@6 = private unnamed_addr constant [25 x i8] c"Choose operation (0-4): \00", align 1
@7 = private unnamed_addr constant [88 x i8] c"Error: value %lld out of bounds for int32 (must be between -2147483648 and 2147483647)\0A\00", align 1
@stderr = external global ptr
@8 = private unnamed_addr constant [9 x i8] c"Goodbye!\00", align 1
@9 = private unnamed_addr constant [21 x i8] c"Enter first number: \00", align 1
@10 = private unnamed_addr constant [22 x i8] c"Enter second number: \00", align 1
@11 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@12 = private unnamed_addr constant [25 x i8] c"Error: Division by zero!\00", align 1
@13 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@14 = private unnamed_addr constant [9 x i8] c"Result: \00", align 1
@15 = private unnamed_addr constant [3 x i8] c"%g\00", align 1
@16 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@17 = private unnamed_addr constant [34 x i8] c"Invalid choice! Please enter 0-4.\00", align 1
@18 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1

define i1 @__user_main() {
entry:
  %error = alloca ptr, align 8
  %result = alloca double, align 8
  %num2 = alloca double, align 8
  %num1 = alloca double, align 8
  %choice = alloca i32, align 4
  %running = alloca i1, align 1
  call void @io_println_str(ptr @0)
  call void @io_println_str(ptr @1)
  call void @io_println_str(ptr @2)
  call void @io_println_str(ptr @3)
  call void @io_println_str(ptr @4)
  call void @io_println_str(ptr @5)
  store i1 true, ptr %running, align 1
  br label %while.condition

while.condition:                                  ; preds = %ifcont45, %entry
  %running1 = load i1, ptr %running, align 1
  br i1 %running1, label %while.body, label %while.end

while.body:                                       ; preds = %while.condition
  call void @io_print_str(ptr @6)
  %0 = call i64 @io_read_int()
  %bounds_sge_min = icmp sge i64 %0, -2147483648
  %bounds_sle_max = icmp sle i64 %0, 2147483647
  %bounds_check = and i1 %bounds_sge_min, %bounds_sle_max
  br i1 %bounds_check, label %bounds_ok, label %bounds_error

bounds_error:                                     ; preds = %while.body
  %1 = load ptr, ptr @stderr, align 8
  %2 = call i32 (ptr, ptr, ...) @fprintf(ptr %1, ptr @7, i64 %0)
  call void @exit(i32 1)
  unreachable

bounds_ok:                                        ; preds = %while.body
  %3 = trunc i64 %0 to i32
  store i32 %3, ptr %choice, align 4
  %choice2 = load i32, ptr %choice, align 4
  %4 = sext i32 %choice2 to i64
  %eqtmp = icmp eq i64 %4, 0
  br i1 %eqtmp, label %then, label %else

then:                                             ; preds = %bounds_ok
  call void @io_println_str(ptr @8)
  store i1 false, ptr %running, align 1
  br label %ifcont45

else:                                             ; preds = %bounds_ok
  %choice3 = load i32, ptr %choice, align 4
  %5 = sext i32 %choice3 to i64
  %getmp = icmp sge i64 %5, 1
  %choice4 = load i32, ptr %choice, align 4
  %6 = sext i32 %choice4 to i64
  %letmp = icmp sle i64 %6, 4
  %andtmp = and i1 %getmp, %letmp
  br i1 %andtmp, label %then5, label %else43

then5:                                            ; preds = %else
  call void @io_print_str(ptr @9)
  %7 = call i64 @io_read_int()
  %8 = sitofp i64 %7 to double
  store double %8, ptr %num1, align 8
  call void @io_print_str(ptr @10)
  %9 = call i64 @io_read_int()
  %10 = sitofp i64 %9 to double
  store double %10, ptr %num2, align 8
  store double 0.000000e+00, ptr %result, align 8
  store ptr @11, ptr %error, align 8
  %choice6 = load i32, ptr %choice, align 4
  %11 = sext i32 %choice6 to i64
  %eqtmp7 = icmp eq i64 %11, 1
  br i1 %eqtmp7, label %then8, label %else11

then8:                                            ; preds = %then5
  %num19 = load double, ptr %num1, align 8
  %num210 = load double, ptr %num2, align 8
  %addtmp = fadd double %num19, %num210
  store double %addtmp, ptr %result, align 8
  br label %ifcont36

else11:                                           ; preds = %then5
  %choice12 = load i32, ptr %choice, align 4
  %12 = sext i32 %choice12 to i64
  %eqtmp13 = icmp eq i64 %12, 2
  br i1 %eqtmp13, label %then14, label %else17

then14:                                           ; preds = %else11
  %num115 = load double, ptr %num1, align 8
  %num216 = load double, ptr %num2, align 8
  %subtmp = fsub double %num115, %num216
  store double %subtmp, ptr %result, align 8
  br label %ifcont35

else17:                                           ; preds = %else11
  %choice18 = load i32, ptr %choice, align 4
  %13 = sext i32 %choice18 to i64
  %eqtmp19 = icmp eq i64 %13, 3
  br i1 %eqtmp19, label %then20, label %else23

then20:                                           ; preds = %else17
  %num121 = load double, ptr %num1, align 8
  %num222 = load double, ptr %num2, align 8
  %multmp = fmul double %num121, %num222
  store double %multmp, ptr %result, align 8
  br label %ifcont34

else23:                                           ; preds = %else17
  %choice24 = load i32, ptr %choice, align 4
  %14 = sext i32 %choice24 to i64
  %eqtmp25 = icmp eq i64 %14, 4
  br i1 %eqtmp25, label %then26, label %ifcont33

then26:                                           ; preds = %else23
  %num227 = load double, ptr %num2, align 8
  %eqtmp28 = fcmp oeq double %num227, 0.000000e+00
  br i1 %eqtmp28, label %then29, label %else30

then29:                                           ; preds = %then26
  store ptr @12, ptr %error, align 8
  br label %ifcont

else30:                                           ; preds = %then26
  %num131 = load double, ptr %num1, align 8
  %num232 = load double, ptr %num2, align 8
  %divtmp = fdiv double %num131, %num232
  store double %divtmp, ptr %result, align 8
  br label %ifcont

ifcont:                                           ; preds = %else30, %then29
  br label %ifcont33

ifcont33:                                         ; preds = %ifcont, %else23
  br label %ifcont34

ifcont34:                                         ; preds = %ifcont33, %then20
  br label %ifcont35

ifcont35:                                         ; preds = %ifcont34, %then14
  br label %ifcont36

ifcont36:                                         ; preds = %ifcont35, %then8
  %error37 = load ptr, ptr %error, align 8
  %netmp = icmp ne ptr %error37, @13
  br i1 %netmp, label %then38, label %else40

then38:                                           ; preds = %ifcont36
  %error39 = load ptr, ptr %error, align 8
  call void @io_println_str(ptr %error39)
  br label %ifcont42

else40:                                           ; preds = %ifcont36
  %result41 = load double, ptr %result, align 8
  %str_buffer = alloca i8, i64 64, align 1
  %15 = call i32 (ptr, ptr, ...) @sprintf(ptr %str_buffer, ptr @15, double %result41)
  %16 = call i64 @strlen(ptr @14)
  %17 = call i64 @strlen(ptr %str_buffer)
  %18 = add i64 %16, %17
  %19 = add i64 %18, 1
  %20 = call ptr @malloc(i64 %19)
  %21 = call ptr @strcpy(ptr %20, ptr @14)
  %22 = call ptr @strcat(ptr %20, ptr %str_buffer)
  call void @io_println_str(ptr %20)
  br label %ifcont42

ifcont42:                                         ; preds = %else40, %then38
  call void @io_println_str(ptr @16)
  br label %ifcont44

else43:                                           ; preds = %else
  call void @io_println_str(ptr @17)
  call void @io_println_str(ptr @18)
  br label %ifcont44

ifcont44:                                         ; preds = %else43, %ifcont42
  br label %ifcont45

ifcont45:                                         ; preds = %ifcont44, %then
  br label %while.condition

while.end:                                        ; preds = %while.condition
  ret i1 false
}

declare void @io_println_str(ptr)

declare void @io_print_str(ptr)

declare i64 @io_read_int()

declare i32 @fprintf(ptr, ptr, ...)

declare void @exit(i32)

declare i32 @sprintf(ptr, ptr, ...)

declare i64 @strlen(ptr)

declare ptr @malloc(i64)

declare ptr @strcpy(ptr, ptr)

declare ptr @strcat(ptr, ptr)

define i32 @main() {
entry:
  %0 = call i1 @__user_main()
  %1 = zext i1 %0 to i32
  ret i32 %1
}
