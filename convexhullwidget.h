#ifndef CONVEXHULLWIDGET_H
#define CONVEXHULLWIDGET_H

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QPainter>

class ConvexHullWidget : public QWidget
{
    Q_OBJECT

private:
    QVector<QPointF> m_points;           //все точки
    QVector<QPointF> m_convexHull;       //точки выпуклой оболочки
    QVector<QPointF> m_concaveHull;      //точки вогнутой оболочки
    double m_gamma;                      //коэффициент глубины (детализации)

    //алгоритм грэхема для построения выпуклой оболочки
    QVector<QPointF> grahamScan(const QVector<QPointF> &points);

    //алгоритм построения вогнутой оболочки
    QVector<QPointF> buildConcaveHullAlgorithm(const QVector<QPointF> &convexHull,
                                               const QVector<QPointF> &allPoints,
                                               double gamma);

    //проверка условия для добавления точки в вогнутую оболочку
    bool satisfiesConcaveCondition(const QPointF &pb, const QPointF &pe,
                                   const QPointF &pi, double gamma);

    //проверка пересечения отрезков
    bool segmentsIntersect(const QPointF &p1, const QPointF &p2,
                           const QPointF &p3, const QPointF &p4);

    //проверка, что треугольник не пересекается с текущей оболочкой
    bool triangleDoesNotIntersectHull(const QPointF &pb, const QPointF &pe,
                                      const QPointF &pi, const QVector<QPointF> &hull);

    //вычисление площади треугольника
    double triangleArea(const QPointF &p1, const QPointF &p2, const QPointF &p3);

    //вычисление ориентации трех точек
    double orientation(const QPointF &p, const QPointF &q, const QPointF &r);

    //вычисление расстояния между двумя точками
    double distance(const QPointF &p1, const QPointF &p2);

    //нахождение самой нижней точки
    int findLowestPoint(const QVector<QPointF> &points);

public:
    explicit ConvexHullWidget(QWidget *parent = nullptr);
    
    //загрузка точек из файла
    bool loadPointsFromFile(const QString &filename);
    
    //построение выпуклой оболочки
    void buildConvexHull();
    
    //построение вогнутой оболочки
    void buildConcaveHull(double gamma);
    
    //установка коэф гамма
    void setGamma(double gamma);

    //Сохранение результатов
    bool saveConcaveHullToFile(const QString &filePath) const;

protected:
    //рисуем то, что загрузили из файла
    void paintEvent(QPaintEvent *event) override;

};

#endif // CONVEXHULLWIDGET_H
