template <typename T>

void logPrintln(T data) 
{
#ifdef SERIAL_ENABLED
    Serial.println(data);
#endif  
}

void logPrintf(const char *format, ...) {
#ifdef SERIAL_ENABLED
  char buffer[128]; // Buffer per contenere la stringa finale formattata
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  
  Serial.print(buffer); // Stampa la stringa risultante
#endif
}