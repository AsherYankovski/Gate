import serial
import time
import serial.tools.list_ports

# --- Настройки ---
ALLOWED_UIDS = {"12345ABC", "67890DEF", "54321XYZ"}  # Добавьте свои UID
BAUD_RATE = 115200
TIMEOUT_SECONDS = 10  # Тайм-аут в секундах для ожидания данных


# --- Функция для поиска доступного порта ---
def find_serial_port():
    ports = list(serial.tools.list_ports.comports())
    for port in ports:
        if "tty" in port.device:  # Проверка на наличие последовательного порта
            return port.device
    return None


# --- Основная программа ---
def main():
    SERIAL_PORT = find_serial_port()
    if not SERIAL_PORT:
        print("Ошибка: Последовательный порт не найден!")
        return

    print(f"Используем последовательный порт: {SERIAL_PORT}")

    try:
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
            print("Сервер Raspberry Pi запущен...")

            while True:
                # Тайм-аут на уровне основного цикла
                timeout_start = time.time()

                while True:
                    # Проверяем наличие данных в последовательном порте
                    if ser.in_waiting > 0:
                        uid = ser.readline().decode("utf-8").strip()  # Читаем UID
                        print(f"Получен UID: {uid}")

                        # Проверяем UID в базе данных
                        if uid in ALLOWED_UIDS:
                            print("Доступ разрешен")
                            ser.write(b"GRANTED\n")  # Отправляем ответ Arduino
                        else:
                            print("Доступ запрещен")
                            ser.write(b"DENIED\n")  # Отправляем ответ Arduino
                        break  # Возврат к главному циклу

                    # Проверяем тайм-аут
                    if time.time() - timeout_start > TIMEOUT_SECONDS:
                        print("Тайм-аут: Нет данных. Перезапуск ожидания...")
                        break
    except Exception as e:
        print(f"Ошибка: {e}")


if __name__ == "__main__":
    main()
