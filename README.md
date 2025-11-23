# Лабораторна робота №3 — FreeRTOS Queues (Variant 9)

## Варіант
- IDX = 9  
- 2 задачі  
- Тип даних: `struct { char ch; uint16_t data; }`  
- Тайм-аут send: 100 мс  
- Тайм-аут recv: 300 мс  
- Додаткове: Consumer рахує середнє за останні 5

## Що робить проєкт
Producer формує структуру `{ch, data}` та відправляє її у чергу.  
Consumer приймає елементи і обчислює кожне середнє останніх 5 значень.  
Вивід ілюструє передачу даних та правильність обчислення середнього значення.

## Як запустити
1. Додати у `main.c`:
```c
extern void vStartLab3_Variant9(void);
vStartLab3_Variant9();
vTaskStartScheduler();
2. Додати файл:
lab3_variant9.c
3. Зібрати WIN32-MSVС / MinGW → запустити.
