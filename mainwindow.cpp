#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QStatusBar>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QDateTime>
#include <QDir>
#include "convexhullwidget.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
    setWindowTitle("Построение вогнутой оболочки");
	setGeometry(100, 100, 1200, 800);

	QWidget *centralWidget = new QWidget(this);
	setCentralWidget(centralWidget);

	QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

	hullWidget = new ConvexHullWidget(this);
	mainLayout->addWidget(hullWidget);

	QHBoxLayout *controlLayout = new QHBoxLayout();

	QPushButton *loadButton = new QPushButton("Загрузить файл", this);
	connect(loadButton, &QPushButton::clicked, this, &MainWindow::loadFile);
	controlLayout->addWidget(loadButton);

	QLabel *gammaLabel = new QLabel("Gamma:", this);
	controlLayout->addWidget(gammaLabel);

	gammaSpinBox = new QDoubleSpinBox(this);
	gammaSpinBox->setRange(0.0, 2.0);
    gammaSpinBox->setValue(0.0);
	gammaSpinBox->setSingleStep(0.1);
	gammaSpinBox->setDecimals(2);
	controlLayout->addWidget(gammaSpinBox);

	QPushButton *calculateButton = new QPushButton("Рассчитать", this);
	connect(calculateButton, &QPushButton::clicked, this, &MainWindow::calculateHull);
	controlLayout->addWidget(calculateButton);

	QPushButton *saveButton = new QPushButton("Сохранить результат", this);
	connect(saveButton, &QPushButton::clicked, this, &MainWindow::saveResult);
	controlLayout->addWidget(saveButton);

	mainLayout->addLayout(controlLayout);

	statusBar()->showMessage("Готов к работе");
}

void MainWindow::loadFile()
{
	QString filename = QFileDialog::getOpenFileName(this, "Выберите файл с точками", "", "Text files (*.txt)");
	if (!filename.isEmpty()) {
		if (hullWidget->loadPointsFromFile(filename)) {
			statusBar()->showMessage("Файл загружен успешно");
		} else {
			statusBar()->showMessage("Ошибка загрузки файла");
		}
	}
}

void MainWindow::calculateHull()
{
	double gamma = gammaSpinBox->value();
	hullWidget->setGamma(gamma);
	statusBar()->showMessage("Расчет завершен");
}

void MainWindow::saveResult()
{
    //формирование result
	QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
	QString filename = QDir::currentPath() + QDir::separator() + QString("result_%1.txt").arg(ts);
	if (hullWidget->saveConcaveHullToFile(filename)) {
		statusBar()->showMessage(QString("Результат сохранен: %1").arg(filename));
	} else {
		statusBar()->showMessage("Не удалось сохранить результат");
	}
}
