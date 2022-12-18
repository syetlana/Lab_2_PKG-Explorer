#include <QApplication>
#include <QPushButton>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableView>
#include <QMenuBar>
#include <QAction>
#include <QDebug>
#include <QFileDialog>
#include <QVBoxLayout>
#include <cmath>
#include "exif.h"

// Виджет-таблица со списком файлов и их свойствами
QTableWidget *m_pTableWidget;

// Считывание информации о заданном списке файлов и заполнение
// виджета таблицы
void fillInfo(QStringList fileNames)
{
    int initialSize = m_pTableWidget->rowCount();
    m_pTableWidget->setRowCount(initialSize + fileNames.size());

    for (int i=initialSize; i<m_pTableWidget->rowCount(); i++)
    {
        int idx = i - initialSize;
        // Базовую информацию получим с помощью QImage
        QImage img;
        img.load(fileNames[idx]);
        QTableWidgetItem *item = new QTableWidgetItem(QFileInfo(fileNames[idx].trimmed()).fileName());
        m_pTableWidget->setItem(i, 0, item);

        QTableWidgetItem *item2 = new QTableWidgetItem(QString::number(img.height()));
        m_pTableWidget->setItem(i, 1, item2);

        QTableWidgetItem *item3 = new QTableWidgetItem(QString::number(img.width()));
        m_pTableWidget->setItem(i, 2, item3);

        QTableWidgetItem *item4 = new QTableWidgetItem(QString::number(img.bitPlaneCount()));
        m_pTableWidget->setItem(i, 3, item4);

        // Разрешение QImage выдает в точках на метр, переведем в дюймы
        // Полагаем что по x и y разрешение одинаковое, поэтому просто возьмем X
        QTableWidgetItem *item5 = new QTableWidgetItem(QString::number(std::round(img.dotsPerMeterX()/39.3701)));
        m_pTableWidget->setItem(i, 4, item5);

        QFile file(fileNames[idx]);
        file.open(QIODevice::ReadOnly);
        QTableWidgetItem *item6;

        if (fileNames[idx].toLower().endsWith("bmp") || fileNames[idx].toLower().endsWith("pcx")) {
            item6 = new QTableWidgetItem("N/A");
        } else {
            double pureSize = img.height()*img.width()*img.bitPlaneCount()/8;
            int fileSize = file.size();
            float ratio = (float) ((int)(pureSize / fileSize * 100 + 0.5));
            item6 = new QTableWidgetItem(QString::number(ratio/100));
        }
        m_pTableWidget->setItem(i, 5, item6);

        // Для считывания exif воспользуемся функциями exif.cpp / exif.h
        // https://github.com/mayanklahiri/easyexif - это open-source библиотека для чтения основных
        // характеристик EXIF. Удобна тем, что достаточно положить два файла (exif.cpp и exif.р) в свой проект
        // и можно использовать их функции
        QByteArray data = file.readAll();
        easyexif::EXIFInfo info;
        if (info.parseFrom((unsigned char *)data.data(), data.size())) {
            // не удалось считать exif - либо картинка - не jpg, либо что-то с ней не так,
            // просто продолжим
            continue;
        }
        // Посмотрим какие из полей, доступных через EXIFInfo, не пусты, и добавим их
        QString info_exif = "";
        if (info.Model.size()) {
            info_exif += "Модель камеры        : ";
            info_exif += info.Model.c_str();
            info_exif += "\n";
        }
        if (info.Make.size()) {
            info_exif += "Производитель камеры : ";
            info_exif += info.Make.c_str();
            info_exif += "\n";
        }
        if (info.DateTimeOriginal.size()) {
            info_exif += "Дата/время исходного изображения : ";
            info_exif += info.DateTimeOriginal.c_str();
            info_exif += "\n";
        }
        if (info.ImageDescription.size()) {
            info_exif += "Описание : ";
            info_exif += info.ImageDescription.c_str();
            info_exif += "\n";
        }
        if (info.Copyright.size()) {
            info_exif += "Copyright : ";
            info_exif += info.Copyright.c_str();
            info_exif += "\n";
        }
        if (info.Software.size()) {
            info_exif += "Обработано с помощью программы : ";
            info_exif += info.Software.c_str();
            info_exif += "\n";
        }
        QTableWidgetItem *item7 = new QTableWidgetItem(info_exif);
        m_pTableWidget->setItem(i, 6, item7);
        file.close();
    }

    m_pTableWidget->resizeColumnsToContents();
    m_pTableWidget->resizeRowsToContents();
}

// Реакция на действие по загрузке файлов (если нажали на соответсвующий пункт меню или кнопку)
// - показываем диалог открытия файлов, позволяем выбрать несколько, возвращаем список выбранных
void chooseFiles()
{
    QStringList fileNames;
    fileNames = QFileDialog::getOpenFileNames(NULL, "Images", ".", "Images (*.jpg *.gif *.tif *.tiff *.bmp *.png *.pcx);;All files (*.*)");
    fillInfo(fileNames);
}

int main(int argc, char **argv)
{
    QApplication app (argc, argv);

    QWidget window;
    window.setMinimumSize(800, 600);

    // Два пункта меню - загрузить файлы и выйти
    QAction *exitAct = new QAction("Exit");
    QAction *loadAct = new QAction("Load files");
    QMenuBar *m_pMenu = new QMenuBar(&window);
    QMenu* fileMenu = m_pMenu->addMenu("&File");
    fileMenu->addAction(loadAct);
    fileMenu->addAction(exitAct);

    // Таблица, где отображаем данные
    m_pTableWidget = new QTableWidget(&window);
    m_pTableWidget->setRowCount(argc-1);
    m_pTableWidget->setColumnCount(6);
    QStringList m_TableHeader;
    m_TableHeader<<"File"<<"Height"<<"Width"<<"Bits"<<"DPI"<<"Compression Ratio"<<"EXIF";
    m_pTableWidget->setHorizontalHeaderLabels(m_TableHeader);

    // Вверху окна сделаем большую кнопку для загрузки файлов
    QPushButton *m_pBtn = new QPushButton("Load Files", &window);
    QVBoxLayout *l = new QVBoxLayout(&window);
    l->addWidget(m_pMenu);
    l->addWidget(m_pBtn);
    l->addWidget(m_pTableWidget);

    // Для быстрого тестирования удобно передавать список файлов для отображения в командной строке
    if (argc > 1) {
        QStringList fileNames;
        for (int i=1; i<argc; i++) {
            fileNames.append(argv[i]);
        }
        fillInfo(fileNames);
    }

    // запретим редактировать таблицу
    m_pTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pTableWidget->setShowGrid(true);
    m_pTableWidget->setGeometry(5, 60, 650, 400);

    QObject::connect(exitAct, &QAction::triggered, qApp, &QApplication::quit);
    QObject::connect(loadAct, &QAction::triggered, qApp, chooseFiles);
    QObject::connect(m_pBtn, &QPushButton::clicked, qApp, chooseFiles);

    window.show();
    return app.exec();
}
