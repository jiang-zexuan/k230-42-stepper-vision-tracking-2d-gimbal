#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "../PanViewF407/Core/Inc/debounced_button.h"

static void test_press_requires_stable_low_level_and_emits_once(void)
{
    DebouncedButton button;

    DebouncedButton_Init(&button, false);

    assert(DebouncedButton_Update(&button, false, 0U, 20U) == false);
    assert(DebouncedButton_Update(&button, true, 10U, 20U) == false);
    assert(DebouncedButton_Update(&button, true, 29U, 20U) == false);
    assert(DebouncedButton_Update(&button, true, 30U, 20U) == true);
    assert(DebouncedButton_Update(&button, true, 40U, 20U) == false);
    assert(DebouncedButton_Update(&button, false, 50U, 20U) == false);
    assert(DebouncedButton_Update(&button, true, 60U, 20U) == false);
    assert(DebouncedButton_Update(&button, true, 80U, 20U) == true);
}

int main(void)
{
    test_press_requires_stable_low_level_and_emits_once();
    puts("debounced_button_test: PASS");
    return 0;
}
