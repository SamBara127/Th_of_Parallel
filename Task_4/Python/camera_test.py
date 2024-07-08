import cv2

def main():
    # Захватываем видео с камеры 0
    cap = cv2.VideoCapture(0)

    if not cap.isOpened():
        print("Ошибка: Не удалось открыть камеру.")
        return

    while True:
        # Захватываем кадр
        ret, frame = cap.read()
        
        if not ret:
            print("Ошибка: Не удалось захватить кадр.")
            break

        # Отображаем кадр
        cv2.imshow('Camera', frame)

        # Проверяем нажатие клавиши 'q' для выхода
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    # Освобождаем захват и закрываем окна
    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()