/*
 * main.c
 *
 *  Created on: 19 sep. 2023
 *      Author: Andlun
 */



int main(void)
{

    return 0;
}

// Application entry point / startup logic.
void __attribute__( ( noreturn ) ) call_start_cpu0() {
  main();
  while( 1 ) {}
}
