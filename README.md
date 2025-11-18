# Лабораторна робота №2 — FreeRTOS TLS (Variant 9)

## Варіант
- IDX = 9  
- 3 задачі  
- Режим: busy  
- Burst: 9  
- TLS слоти: 1/0  
- Навантаження: 18400 + 800*i циклів  

## Що робить проєкт
Створює 3 незалежні FreeRTOS-задачі, кожна має власний TLS-контекст і профіль.  
Кожна задача виконує 39 ітерацій, рахує CRC8, формує burst з 9 логів і друкує їх.  
Після завершення задачі чистять TLS і виводять `Task finished`.

## Як запустити
1. Додати у `main.c`:
   ```c
   extern void vStartVariant9(void);
   vStartVariant9();
   vTaskStartScheduler();
2. У FreeRTOSConfig.h:
  #define configNUM_THREAD_LOCAL_STORAGE_POINTERS 2

3. Зібрати WIN32-MSVC / MingW → запустити.
