static volatile unsigned tick;

void _systick(void){
  tick++;
  __asm volatile("sev");
}

unsigned gettick(void){
  return tick;
}

void msleep(unsigned ms){
  unsigned t = tick;
  while(tick - t < ms) __asm volatile("wfe");
}
