#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../PanViewF407/Core/Inc/periodic_task.h"

static void test_task_is_due_at_each_period(void)
{
    PeriodicTask task;

    PeriodicTask_Init(&task, 100U, 0U);

    assert(PeriodicTask_IsDue(&task, 99U) == false);
    assert(PeriodicTask_IsDue(&task, 100U) == true);
    assert(PeriodicTask_IsDue(&task, 100U) == false);
    assert(PeriodicTask_IsDue(&task, 199U) == false);
    assert(PeriodicTask_IsDue(&task, 200U) == true);
}

int main(void)
{
    test_task_is_due_at_each_period();
    puts("periodic_task_test: PASS");
    return 0;
}
