; ModuleID = 'summit'
source_filename = "summit"

@0 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@1 = private unnamed_addr constant [18 x i8] c"Values are equal.\00", align 1
@2 = private unnamed_addr constant [9 x i8] c"positive\00", align 1
@3 = private unnamed_addr constant [9 x i8] c"negative\00", align 1
@4 = private unnamed_addr constant [5 x i8] c"zero\00", align 1
@5 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@6 = private unnamed_addr constant [7 x i8] c"Summit\00", align 1
@7 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@8 = private unnamed_addr constant [16 x i8] c"Welcome to the \00", align 1
@9 = private unnamed_addr constant [7 x i8] c" demo!\00", align 1
@10 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@11 = private unnamed_addr constant [5 x i8] c"x = \00", align 1
@12 = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@13 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@14 = private unnamed_addr constant [5 x i8] c"y = \00", align 1
@15 = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@16 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@17 = private unnamed_addr constant [9 x i8] c"x + y = \00", align 1
@18 = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@19 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@20 = private unnamed_addr constant [9 x i8] c"x - y = \00", align 1
@21 = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@22 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@23 = private unnamed_addr constant [9 x i8] c"x * y = \00", align 1
@24 = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@25 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@26 = private unnamed_addr constant [13 x i8] c"max(x, y) = \00", align 1
@27 = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@28 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@29 = private unnamed_addr constant [10 x i8] c"x - y is \00", align 1

declare i32 @printf(ptr, ...)

define i32 @add(i32 %a, i32 %b) {
entry:
  %sum = alloca i32, align 4
  %a1 = alloca i32, align 4
  store i32 %a, ptr %a1, align 4
  %b2 = alloca i32, align 4
  store i32 %b, ptr %b2, align 4
  %a3 = load i32, ptr %a1, align 4
  %b4 = load i32, ptr %b2, align 4
  %addtmp = add i32 %a3, %b4
  store i32 %addtmp, ptr %sum, align 4
  %sum5 = load i32, ptr %sum, align 4
  ret i32 %sum5
}

define i32 @subtract(i32 %a, i32 %b) {
entry:
  %diff = alloca i32, align 4
  %a1 = alloca i32, align 4
  store i32 %a, ptr %a1, align 4
  %b2 = alloca i32, align 4
  store i32 %b, ptr %b2, align 4
  %a3 = load i32, ptr %a1, align 4
  %b4 = load i32, ptr %b2, align 4
  %subtmp = sub i32 %a3, %b4
  store i32 %subtmp, ptr %diff, align 4
  %diff5 = load i32, ptr %diff, align 4
  ret i32 %diff5
}

define i32 @multiply(i32 %a, i32 %b) {
entry:
  %product = alloca i32, align 4
  %a1 = alloca i32, align 4
  store i32 %a, ptr %a1, align 4
  %b2 = alloca i32, align 4
  store i32 %b, ptr %b2, align 4
  %a3 = load i32, ptr %a1, align 4
  %b4 = load i32, ptr %b2, align 4
  %multmp = mul i32 %a3, %b4
  store i32 %multmp, ptr %product, align 4
  %product5 = load i32, ptr %product, align 4
  ret i32 %product5
}

define i32 @max(i32 %a, i32 %b) {
entry:
  %a1 = alloca i32, align 4
  store i32 %a, ptr %a1, align 4
  %b2 = alloca i32, align 4
  store i32 %b, ptr %b2, align 4
  %a3 = load i32, ptr %a1, align 4
  %b4 = load i32, ptr %b2, align 4
  %gttmp = icmp sgt i32 %a3, %b4
  br i1 %gttmp, label %then, label %else

then:                                             ; preds = %entry
  %a5 = load i32, ptr %a1, align 4
  ret i32 %a5

else:                                             ; preds = %entry
  %a6 = load i32, ptr %a1, align 4
  %b7 = load i32, ptr %b2, align 4
  %lttmp = icmp slt i32 %a6, %b7
  br i1 %lttmp, label %then8, label %else10

then8:                                            ; preds = %else
  %b9 = load i32, ptr %b2, align 4
  ret i32 %b9

else10:                                           ; preds = %else
  %0 = call i32 (ptr, ...) @printf(ptr @0, ptr @1)
  %a11 = load i32, ptr %a1, align 4
  ret i32 %a11

ifcont:                                           ; No predecessors!
  br label %ifcont12

ifcont12:                                         ; preds = %ifcont
  ret i32 0
}

define ptr @describe_number(i32 %n) {
entry:
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  %n2 = load i32, ptr %n1, align 4
  %0 = sext i32 %n2 to i64
  %gttmp = icmp sgt i64 %0, 0
  br i1 %gttmp, label %then, label %else

then:                                             ; preds = %entry
  ret ptr @2

else:                                             ; preds = %entry
  %n3 = load i32, ptr %n1, align 4
  %1 = sext i32 %n3 to i64
  %lttmp = icmp slt i64 %1, 0
  br i1 %lttmp, label %then4, label %else5

then4:                                            ; preds = %else
  ret ptr @3

else5:                                            ; preds = %else
  ret ptr @4

ifcont:                                           ; No predecessors!
  br label %ifcont6

ifcont6:                                          ; preds = %ifcont
  ret ptr @5
}

define i1 @__entry_start() {
entry:
  %desc = alloca ptr, align 8
  %largest = alloca i32, align 4
  %m = alloca i32, align 4
  %d = alloca i32, align 4
  %s = alloca i32, align 4
  %y = alloca i32, align 4
  %x = alloca i32, align 4
  %name = alloca ptr, align 8
  store ptr @6, ptr %name, align 8
  %name1 = load ptr, ptr %name, align 8
  %0 = call i64 @strlen(ptr @8)
  %1 = call i64 @strlen(ptr %name1)
  %2 = add i64 %0, %1
  %3 = add i64 %2, 1
  %4 = call ptr @malloc(i64 %3)
  %5 = call ptr @strcpy(ptr %4, ptr @8)
  %6 = call ptr @strcat(ptr %4, ptr %name1)
  %7 = call i64 @strlen(ptr %4)
  %8 = call i64 @strlen(ptr @9)
  %9 = add i64 %7, %8
  %10 = add i64 %9, 1
  %11 = call ptr @malloc(i64 %10)
  %12 = call ptr @strcpy(ptr %11, ptr %4)
  %13 = call ptr @strcat(ptr %11, ptr @9)
  %14 = call i32 (ptr, ...) @printf(ptr @7, ptr %11)
  store i32 10, ptr %x, align 4
  store i32 5, ptr %y, align 4
  %x2 = load i32, ptr %x, align 4
  %str_buffer = alloca i8, i64 64, align 1
  %15 = call i32 (ptr, ptr, ...) @sprintf(ptr %str_buffer, ptr @12, i32 %x2)
  %16 = call i64 @strlen(ptr @11)
  %17 = call i64 @strlen(ptr %str_buffer)
  %18 = add i64 %16, %17
  %19 = add i64 %18, 1
  %20 = call ptr @malloc(i64 %19)
  %21 = call ptr @strcpy(ptr %20, ptr @11)
  %22 = call ptr @strcat(ptr %20, ptr %str_buffer)
  %23 = call i32 (ptr, ...) @printf(ptr @10, ptr %20)
  %y3 = load i32, ptr %y, align 4
  %str_buffer4 = alloca i8, i64 64, align 1
  %24 = call i32 (ptr, ptr, ...) @sprintf(ptr %str_buffer4, ptr @15, i32 %y3)
  %25 = call i64 @strlen(ptr @14)
  %26 = call i64 @strlen(ptr %str_buffer4)
  %27 = add i64 %25, %26
  %28 = add i64 %27, 1
  %29 = call ptr @malloc(i64 %28)
  %30 = call ptr @strcpy(ptr %29, ptr @14)
  %31 = call ptr @strcat(ptr %29, ptr %str_buffer4)
  %32 = call i32 (ptr, ...) @printf(ptr @13, ptr %29)
  %x5 = load i32, ptr %x, align 4
  %y6 = load i32, ptr %y, align 4
  %33 = call i32 @add(i32 %x5, i32 %y6)
  store i32 %33, ptr %s, align 4
  %s7 = load i32, ptr %s, align 4
  %str_buffer8 = alloca i8, i64 64, align 1
  %34 = call i32 (ptr, ptr, ...) @sprintf(ptr %str_buffer8, ptr @18, i32 %s7)
  %35 = call i64 @strlen(ptr @17)
  %36 = call i64 @strlen(ptr %str_buffer8)
  %37 = add i64 %35, %36
  %38 = add i64 %37, 1
  %39 = call ptr @malloc(i64 %38)
  %40 = call ptr @strcpy(ptr %39, ptr @17)
  %41 = call ptr @strcat(ptr %39, ptr %str_buffer8)
  %42 = call i32 (ptr, ...) @printf(ptr @16, ptr %39)
  %x9 = load i32, ptr %x, align 4
  %y10 = load i32, ptr %y, align 4
  %43 = call i32 @subtract(i32 %x9, i32 %y10)
  store i32 %43, ptr %d, align 4
  %d11 = load i32, ptr %d, align 4
  %str_buffer12 = alloca i8, i64 64, align 1
  %44 = call i32 (ptr, ptr, ...) @sprintf(ptr %str_buffer12, ptr @21, i32 %d11)
  %45 = call i64 @strlen(ptr @20)
  %46 = call i64 @strlen(ptr %str_buffer12)
  %47 = add i64 %45, %46
  %48 = add i64 %47, 1
  %49 = call ptr @malloc(i64 %48)
  %50 = call ptr @strcpy(ptr %49, ptr @20)
  %51 = call ptr @strcat(ptr %49, ptr %str_buffer12)
  %52 = call i32 (ptr, ...) @printf(ptr @19, ptr %49)
  %x13 = load i32, ptr %x, align 4
  %y14 = load i32, ptr %y, align 4
  %53 = call i32 @multiply(i32 %x13, i32 %y14)
  store i32 %53, ptr %m, align 4
  %m15 = load i32, ptr %m, align 4
  %str_buffer16 = alloca i8, i64 64, align 1
  %54 = call i32 (ptr, ptr, ...) @sprintf(ptr %str_buffer16, ptr @24, i32 %m15)
  %55 = call i64 @strlen(ptr @23)
  %56 = call i64 @strlen(ptr %str_buffer16)
  %57 = add i64 %55, %56
  %58 = add i64 %57, 1
  %59 = call ptr @malloc(i64 %58)
  %60 = call ptr @strcpy(ptr %59, ptr @23)
  %61 = call ptr @strcat(ptr %59, ptr %str_buffer16)
  %62 = call i32 (ptr, ...) @printf(ptr @22, ptr %59)
  %x17 = load i32, ptr %x, align 4
  %y18 = load i32, ptr %y, align 4
  %63 = call i32 @max(i32 %x17, i32 %y18)
  store i32 %63, ptr %largest, align 4
  %largest19 = load i32, ptr %largest, align 4
  %str_buffer20 = alloca i8, i64 64, align 1
  %64 = call i32 (ptr, ptr, ...) @sprintf(ptr %str_buffer20, ptr @27, i32 %largest19)
  %65 = call i64 @strlen(ptr @26)
  %66 = call i64 @strlen(ptr %str_buffer20)
  %67 = add i64 %65, %66
  %68 = add i64 %67, 1
  %69 = call ptr @malloc(i64 %68)
  %70 = call ptr @strcpy(ptr %69, ptr @26)
  %71 = call ptr @strcat(ptr %69, ptr %str_buffer20)
  %72 = call i32 (ptr, ...) @printf(ptr @25, ptr %69)
  %d21 = load i32, ptr %d, align 4
  %73 = call ptr @describe_number(i32 %d21)
  store ptr %73, ptr %desc, align 8
  %desc22 = load ptr, ptr %desc, align 8
  %74 = call i64 @strlen(ptr @29)
  %75 = call i64 @strlen(ptr %desc22)
  %76 = add i64 %74, %75
  %77 = add i64 %76, 1
  %78 = call ptr @malloc(i64 %77)
  %79 = call ptr @strcpy(ptr %78, ptr @29)
  %80 = call ptr @strcat(ptr %78, ptr %desc22)
  %81 = call i32 (ptr, ...) @printf(ptr @28, ptr %78)
  ret i1 false
}

declare i64 @strlen(ptr)

declare ptr @malloc(i64)

declare ptr @strcpy(ptr, ptr)

declare ptr @strcat(ptr, ptr)

declare i32 @sprintf(ptr, ptr, ...)

define i32 @main() {
entry:
  %0 = call i1 @__entry_start()
  %1 = zext i1 %0 to i32
  ret i32 %1
}
