#include <stdio.h>
#include "calculator.h"
#include "student.h"

int main(void)
{
    int a = 20;
    int b = 10;

    printf("Addition: %d\n", add(a, b));
    printf("Subtraction: %d\n", subtract(a, b));
    printf("Student: ");
    display_student();

    return 0;
}