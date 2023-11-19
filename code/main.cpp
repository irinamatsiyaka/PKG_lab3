#include <QApplication>
#include <QMainWindow>
#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPixmap>
#include <QImage>
#include <QDebug>

// Глобальная пороговая обработка: метод Оцу
QImage applyOtsuThreshold(const QImage &inputImage) {
    QImage outputImage = inputImage.convertToFormat(QImage::Format_Grayscale8);

    // Вычисление гистограммы
    QVector<int> histogram(256, 0);
    for (int y = 0; y < inputImage.height(); ++y) {
        for (int x = 0; x < inputImage.width(); ++x) {
            QRgb pixel = inputImage.pixel(x, y);
            int grayValue = qRed(pixel);
            ++histogram[grayValue];
        }
    }

    // Нормализация гистограммы
    QVector<double> normalizedHistogram(256, 0.0);
    for (int i = 0; i < 256; ++i) {
        normalizedHistogram[i] = static_cast<double>(histogram[i]) / (inputImage.width() * inputImage.height());
    }

    // Вычисление порога по методу Оцу
    double maxSigma = 0.0;
    int threshold = 0;
    for (int t = 0; t < 256; ++t) {
        double w0 = 0.0, w1 = 0.0, u0 = 0.0, u1 = 0.0, sigma = 0.0;

        for (int i = 0; i <= t; ++i) {
            w0 += normalizedHistogram[i];
            u0 += i * normalizedHistogram[i];
        }
        for (int i = t + 1; i < 256; ++i) {
            w1 += normalizedHistogram[i];
            u1 += i * normalizedHistogram[i];
        }

        if (w0 != 0 && w1 != 0) {
            u0 /= w0;
            u1 /= w1;
            sigma = w0 * w1 * (u0 - u1) * (u0 - u1);
        }

        if (sigma > maxSigma) {
            maxSigma = sigma;
            threshold = t;
        }
    }

    // Бинаризация изображения по порогу
    for (int y = 0; y < inputImage.height(); ++y) {
        for (int x = 0; x < inputImage.width(); ++x) {
            QRgb pixel = inputImage.pixel(x, y);
            int grayValue = qRed(pixel);
            int newValue = (grayValue > threshold) ? 255 : 0;
            outputImage.setPixel(x, y, qRgb(newValue, newValue, newValue));
        }
    }

    return outputImage;
}

//адаптивная пороговая обработка изображения:
QImage applyAdaptiveThreshold(const QImage &inputImage, int windowSize, int thresholdOffset) {
    QImage outputImage = inputImage;

    for (int y = windowSize / 2; y < inputImage.height() - windowSize / 2; ++y) {
        for (int x = windowSize / 2; x < inputImage.width() - windowSize / 2; ++x) {
            int sum = 0;

            // Вычисляем среднее значение пикселей в локальном окне
            for (int i = -windowSize / 2; i <= windowSize / 2; ++i) {
                for (int j = -windowSize / 2; j <= windowSize / 2; ++j) {
                    QRgb pixel = inputImage.pixel(x + j, y + i);
                    sum += qRed(pixel); // Мы берем только значение красного канала, но можно использовать любой канал
                }
            }

            int avg = sum / (windowSize * windowSize);
            int currentPixelValue = qRed(inputImage.pixel(x, y)); // Текущее значение пикселя

            // Применяем пороговую обработку
            int newPixelValue = (currentPixelValue > avg + thresholdOffset) ? 255 : 0;

            outputImage.setPixel(x, y, qRgb(newPixelValue, newPixelValue, newPixelValue));
        }
    }

    return outputImage;
}


// Глобальная пороговая обработка: бинаризация по среднему значению
QImage applyMeanThreshold(const QImage &inputImage) {
    QImage outputImage = inputImage.convertToFormat(QImage::Format_Grayscale8);

    // Вычисление среднего значения яркости пикселей
    int sum = 0;
    for (int y = 0; y < inputImage.height(); ++y) {
        for (int x = 0; x < inputImage.width(); ++x) {
            QRgb pixel = inputImage.pixel(x, y);
            int grayValue = qRed(pixel);
            sum += grayValue;
        }
    }
    int mean = sum / (inputImage.width() * inputImage.height());

    // Бинаризация изображения по среднему значению
    for (int y = 0; y < inputImage.height(); ++y) {
        for (int x = 0; x < inputImage.width(); ++x) {
            QRgb pixel = inputImage.pixel(x, y);
            int grayValue = qRed(pixel);
            int newValue = (grayValue > mean) ? 255 : 0;
            outputImage.setPixel(x, y, qRgb(newValue, newValue, newValue));
        }
    }

    return outputImage;
}

QImage applyHighPassFilter(const QImage &inputImage) {
    QImage outputImage = inputImage;

    // Матрица фильтра высокочастотной усиливающей маски
    QVector<QVector<int>> highPassFilter = {
        { -1, -1, -1 },
        { -1,  9, -1 },
        { -1, -1, -1 }
    };

    int matrixSize = 3; // Размер матрицы фильтра

    // Применение высокочастотного фильтра
    for (int y = matrixSize / 2; y < inputImage.height() - matrixSize / 2; ++y) {
        for (int x = matrixSize / 2; x < inputImage.width() - matrixSize / 2; ++x) {
            int sumRed = 0, sumGreen = 0, sumBlue = 0;

            // Применение маски фильтра к окрестности каждого пикселя
            for (int i = -matrixSize / 2; i <= matrixSize / 2; ++i) {
                for (int j = -matrixSize / 2; j <= matrixSize / 2; ++j) {
                    QRgb pixel = inputImage.pixel(x + j, y + i);
                    int red = qRed(pixel);
                    int green = qGreen(pixel);
                    int blue = qBlue(pixel);

                    // Умножение значений каналов пикселя на значения из матрицы фильтра
                    sumRed += red * highPassFilter[i + matrixSize / 2][j + matrixSize / 2];
                    sumGreen += green * highPassFilter[i + matrixSize / 2][j + matrixSize / 2];
                    sumBlue += blue * highPassFilter[i + matrixSize / 2][j + matrixSize / 2];
                }
            }

            // Ограничение значений RGB в пределах 0-255
            sumRed = qBound(0, sumRed, 255);
            sumGreen = qBound(0, sumGreen, 255);
            sumBlue = qBound(0, sumBlue, 255);

            // Применение новых значений пикселя к выходному изображению
            outputImage.setPixel(x, y, qRgb(sumRed, sumGreen, sumBlue));
        }
    }

    return outputImage;
}

void loadImageWithScaling(QLabel* label, const QString& fileName) {
    QImage originalImage(fileName);
    if (originalImage.isNull()) {
        qDebug() << "Error: Unable to load image.";
        return;
    }

    // Максимальный размер для отображения
    QSize maxSize(800, 600);

    // Масштабируем изображение, если оно больше максимального размера
    QSize scaledSize = originalImage.size().scaled(maxSize, Qt::KeepAspectRatio);

    QImage scaledImage = originalImage.scaled(scaledSize, Qt::KeepAspectRatio);

    label->setPixmap(QPixmap::fromImage(scaledImage));
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QMainWindow mainWindow;
    QWidget centralWidget;
    QVBoxLayout layout(&centralWidget);
    QLabel labelOriginal;
    QLabel labelProcessed;

    // Добавляем прокрутку для QLabel
    labelOriginal.setScaledContents(true);
    labelProcessed.setScaledContents(true);

    // Устанавливаем максимальные размеры для QLabel
    labelOriginal.setMaximumSize(800, 600); // Установите размеры в соответствии с вашими предпочтениями
    labelProcessed.setMaximumSize(800, 600);

    QPushButton otsuThresholdBtn("Применить метод Отцу");
    QPushButton meanThresholdBtn("Применить метод усреднения");
    QPushButton highPassFilterBtn("Применить высокочастотный фильтр");
    QPushButton adaptiveThresholdBtn("Применить адаптивную пороговую обработку");



    layout.addWidget(&labelOriginal, 0, Qt::AlignCenter);
    layout.addWidget(&labelProcessed, 0, Qt::AlignCenter);
    layout.addWidget(&otsuThresholdBtn, 0, Qt::AlignCenter);
    layout.addWidget(&meanThresholdBtn, 0, Qt::AlignCenter);
    layout.addWidget(&highPassFilterBtn, 0, Qt::AlignCenter);
    layout.addWidget(&adaptiveThresholdBtn, 0, Qt::AlignCenter);


    QObject::connect(&otsuThresholdBtn, &QPushButton::clicked, [&]() {
        QString fileName = QFileDialog::getOpenFileName(&mainWindow, "Open Image", "", "Images (*.png *.jpg *.bmp)");
        if (fileName.isEmpty())
            return;

        QImage originalImage(fileName);
        if (originalImage.isNull()) {
            qDebug() << "Error: Unable to load image.";
            return;
        }

        QImage processedImage = applyOtsuThreshold(originalImage);

        labelOriginal.setPixmap(QPixmap::fromImage(originalImage));
        labelProcessed.setPixmap(QPixmap::fromImage(processedImage));
    });

    QObject::connect(&meanThresholdBtn, &QPushButton::clicked, [&]() {
        QString fileName = QFileDialog::getOpenFileName(&mainWindow, "Open Image", "", "Images (*.png *.jpg *.bmp)");
        if (fileName.isEmpty())
            return;

        QImage originalImage(fileName);
        if (originalImage.isNull()) {
            qDebug() << "Error: Unable to load image.";
            return;
        }

        QImage processedImage = applyMeanThreshold(originalImage);

        labelOriginal.setPixmap(QPixmap::fromImage(originalImage));
        labelProcessed.setPixmap(QPixmap::fromImage(processedImage));
    });

    QObject::connect(&highPassFilterBtn, &QPushButton::clicked, [&]() {
        QString fileName = QFileDialog::getOpenFileName(&mainWindow, "Open Image", "", "Images (*.png *.jpg *.bmp)");
        if (fileName.isEmpty())
            return;

        QImage originalImage(fileName);
        if (originalImage.isNull()) {
            qDebug() << "Error: Unable to load image.";
            return;
        }

        QImage processedImage = applyHighPassFilter(originalImage);

        labelOriginal.setPixmap(QPixmap::fromImage(originalImage));
        labelProcessed.setPixmap(QPixmap::fromImage(processedImage));
    });

    QObject::connect(&adaptiveThresholdBtn, &QPushButton::clicked, [&]() {
        QString fileName = QFileDialog::getOpenFileName(&mainWindow, "Open Image", "", "Images (*.png *.jpg *.bmp)");
        if (fileName.isEmpty())
            return;

        QImage originalImage(fileName);
        if (originalImage.isNull()) {
            qDebug() << "Error: Unable to load image.";
            return;
        }

        int windowSize = 5; // Размер окна для адаптивной пороговой обработки
        int thresholdOffset = 3; // Смещение порога

        QImage processedImage = applyAdaptiveThreshold(originalImage, windowSize, thresholdOffset);

        labelOriginal.setPixmap(QPixmap::fromImage(originalImage));
        labelProcessed.setPixmap(QPixmap::fromImage(processedImage));
    });


    mainWindow.setCentralWidget(&centralWidget);
    mainWindow.show();

    return app.exec();
}
