esp_timer_handle_t sysTimer;

void initTimer(void)
{
  const esp_timer_create_args_t timer_args = {
    .callback = &TimerCallback,
    .name = "sysTimer"
  };
  esp_timer_create(&timer_args, &sysTimer);
  esp_timer_start_periodic(sysTimer, 1000); // 1ms
}

void startTimer(void)
{
  esp_timer_start_periodic(sysTimer, 1000);
}

void stopTimer(void)
{
  esp_timer_stop(sysTimer);
}