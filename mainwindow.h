#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDoubleSpinBox>

class ConvexHullWidget;

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(QWidget *parent = nullptr);

private slots:
	void loadFile();
	void calculateHull();
	void saveResult();

private:
	ConvexHullWidget *hullWidget;
	QDoubleSpinBox *gammaSpinBox;
};

#endif // MAINWINDOW_H

