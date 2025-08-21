#include "convexhullwidget.h"
#include <QPainter>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QMessageBox>
#include <QDateTime>
#include <algorithm>
#include <cmath>
#include <QtConcurrent>
#include <QFuture>
#include <QThread>

ConvexHullWidget::ConvexHullWidget(QWidget *parent)
    : QWidget(parent)
    , m_gamma(0.0)
{
    setMinimumSize(600, 400);
    setWindowTitle("Task 3");
}

bool ConvexHullWidget::loadPointsFromFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл: " + filename);
        return false;
    }

    m_points.clear();
    m_convexHull.clear();
    m_concaveHull.clear();
    QTextStream in(&file);
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        
        QStringList parts = line.split(" ", Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            bool ok1, ok2;
            double x = parts[0].toDouble(&ok1);
            double y = parts[1].toDouble(&ok2);
            
            if (ok1 && ok2) {
                m_points.append(QPointF(x, y));
            }
        }
    }
    
    file.close();
    
    if (m_points.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Файл не содержит корректных точек");
        return false;
    }
    
    qDebug() << "Загружено" << m_points.size() << "точек";
    buildConvexHull();
    m_gamma = 0.0;
    buildConcaveHull(m_gamma);
    update();
    return true;
}

void ConvexHullWidget::buildConvexHull()
{
    if (m_points.size() < 3) {
        m_convexHull = m_points;
        return;
    }
    
    m_convexHull = grahamScan(m_points);
}

void ConvexHullWidget::buildConcaveHull(double gamma)
{
    if (gamma < 0.0) gamma = 0.0;
    if (gamma > 2.0) gamma = 2.0;

    if (m_convexHull.size() < 3) {
        m_concaveHull = m_convexHull;
        return;
    }

    m_concaveHull = buildConcaveHullAlgorithm(m_convexHull, m_points, gamma);
}

void ConvexHullWidget::setGamma(double gamma)
{
    if (gamma < 0.0) gamma = 0.0;
    if (gamma > 2.0) gamma = 2.0;
    m_gamma = gamma;
    if (!m_points.isEmpty()) {
        buildConcaveHull(m_gamma);
        update();
    }
}

bool ConvexHullWidget::saveConcaveHullToFile(const QString &filePath) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream out(&f);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(6);
    for (const QPointF &p : m_concaveHull) {
        out << p.x() << " " << p.y() << "\n";
    }
    f.close();
    return true;
}

QVector<QPointF> ConvexHullWidget::grahamScan(const QVector<QPointF> &points)
{
    if (points.size() < 3) {
        return points;
    }
    
    QVector<QPointF> sortedPoints = points;
    
    //нахождение самой нижней точки
    int lowestIndex = findLowestPoint(sortedPoints);
    std::swap(sortedPoints[0], sortedPoints[lowestIndex]);
    
    //сортировка оставшихся точек по полярному углу
    QPointF p0 = sortedPoints[0];
    std::sort(sortedPoints.begin() + 1, sortedPoints.end(), 
        [p0, this](const QPointF &a, const QPointF &b) {
            double orient = orientation(p0, a, b);
            if (std::abs(orient) < 1e-9) {
                //если коллинеарны, то сорт по дист
                return distance(p0, a) < distance(p0, b);
            }
            return orient > 0;
        });
    
    //если точка дубликат - удалить
    auto it = std::unique(sortedPoints.begin() + 1, sortedPoints.end(),
        [p0, this](const QPointF &a, const QPointF &b) {
            return std::abs(orientation(p0, a, b)) < 1e-9;
        });
    sortedPoints.erase(it, sortedPoints.end());
    
    if (sortedPoints.size() < 3) {
        return sortedPoints;
    }
    
    //строим выпуклую оболочку
    QVector<QPointF> hull;
    hull.push_back(sortedPoints[0]);
    hull.push_back(sortedPoints[1]);
    
    for (int i = 2; i < sortedPoints.size(); ++i) {
        while (hull.size() > 1 && orientation(hull[hull.size() - 2], hull[hull.size() - 1], sortedPoints[i]) <= 0) {
            hull.pop_back();
        }
        hull.push_back(sortedPoints[i]);
    }
    
    return hull;
}

QVector<QPointF> ConvexHullWidget::buildConcaveHullAlgorithm(const QVector<QPointF> &convexHull, 
                                                            const QVector<QPointF> &allPoints, 
                                                            double gamma)
{
    QVector<QPointF> hull = convexHull;
    
    //создание множества точек, не входящих в выпуклую оболочку
    QVector<QPointF> remainingPoints;
    for (const QPointF &point : allPoints) {
        bool inHull = false;
        for (const QPointF &hullPoint : hull) {
            if (std::abs(point.x() - hullPoint.x()) < 1e-9 && 
                std::abs(point.y() - hullPoint.y()) < 1e-9) {
                inHull = true;
                break;
            }
        }
        if (!inHull) {
            remainingPoints.append(point);
        }
    }
    
    bool changed = true;
    while (changed) {
        changed = false;
        
        //нахождение стороны с максимальной длиной
        int maxSideIndex = 0;
        double maxLength = 0;
        
        for (int i = 0; i < hull.size(); ++i) {
            int next = (i + 1) % hull.size();
            double sideLength = distance(hull[i], hull[next]);
            if (sideLength > maxLength) {
                maxLength = sideLength;
                maxSideIndex = i;
            }
        }
        
        QPointF pb = hull[maxSideIndex];
        QPointF pe = hull[(maxSideIndex + 1) % hull.size()];
        
        //поиск подходящей точки для создания вогнутости
        QPointF bestPoint;
        double minArea = std::numeric_limits<double>::max();
        bool found = false;
        
        //многопоточность для поиска лучшей точки
        if (remainingPoints.size() > 100) {
            //разбиение поиска на параллельные потоки
            const int numThreads = QThread::idealThreadCount();
            const int chunkSize = remainingPoints.size() / numThreads;
            
            QVector<QFuture<QPair<QPointF, double>>> futures;
            
            for (int i = 0; i < numThreads; ++i) {
                int start = i * chunkSize;
                int end = (i == numThreads - 1) ? remainingPoints.size() : (i + 1) * chunkSize;
                
                QFuture<QPair<QPointF, double>> future = QtConcurrent::run([=, &remainingPoints, &hull]() {
                    QPointF localBestPoint;
                    double localMinArea = std::numeric_limits<double>::max();
                    bool localFound = false;
                    
                    for (int j = start; j < end; ++j) {
                        const QPointF &pi = remainingPoints[j];
                        
                        if (satisfiesConcaveCondition(pb, pe, pi, gamma)) {
                            if (triangleDoesNotIntersectHull(pb, pe, pi, hull)) {
                                double area = triangleArea(pb, pe, pi);
                                if (area < localMinArea) {
                                    localMinArea = area;
                                    localBestPoint = pi;
                                    localFound = true;
                                }
                            }
                        }
                    }
                    
                    return qMakePair(localBestPoint, localFound ? localMinArea : std::numeric_limits<double>::max());
                });
                
                futures.append(future);
            }
            
            for (QFuture<QPair<QPointF, double>> &future : futures) {
                future.waitForFinished();
                QPair<QPointF, double> result = future.result();
                
                if (result.second < minArea) {
                    minArea = result.second;
                    bestPoint = result.first;
                    found = true;
                }
            }
        } else {
            //для малого кол-ва точек - обычный поиск
            for (const QPointF &pi : remainingPoints) {
                //условие вогнутости
                if (satisfiesConcaveCondition(pb, pe, pi, gamma)) {
                    //если треугольник не пересекается с текущей оболочкой
                    if (triangleDoesNotIntersectHull(pb, pe, pi, hull)) {
                        double area = triangleArea(pb, pe, pi);
                        if (area < minArea) {
                            minArea = area;
                            bestPoint = pi;
                            found = true;
                        }
                    }
                }
            }
        }
        
        if (found) {
            hull.insert(hull.begin() + maxSideIndex + 1, bestPoint);
            auto it = std::find_if(remainingPoints.begin(), remainingPoints.end(),
                [&bestPoint](const QPointF &p) {
                    return std::abs(p.x() - bestPoint.x()) < 1e-9 && 
                           std::abs(p.y() - bestPoint.y()) < 1e-9;
                });
            if (it != remainingPoints.end()) {
                remainingPoints.erase(it);
            }
            
            changed = true;
        }
    }
    
    return hull;
}

bool ConvexHullWidget::satisfiesConcaveCondition(const QPointF &pb, const QPointF &pe, 
                                                const QPointF &pi, double gamma)
{
    double d0 = distance(pb, pe);
    double d1 = distance(pb, pi);
    double d2 = distance(pe, pi);
    
    //d1^2 + d2^2 - d0^2 < gamma * min(d1^2, d2^2)
    double leftSide = d1 + d2 - d0;
    double rightSide = gamma * std::min(d1, d2);
    
    return leftSide < rightSide;
}

bool ConvexHullWidget::segmentsIntersect(const QPointF &p1, const QPointF &p2, 
                                        const QPointF &p3, const QPointF &p4)
{
    double o1 = orientation(p1, p2, p3);
    double o2 = orientation(p1, p2, p4);
    double o3 = orientation(p3, p4, p1);
    double o4 = orientation(p3, p4, p2);
    
    //отрезки пересекаются, если ориентации разные
    if (o1 != 0 && o2 != 0 && o3 != 0 && o4 != 0) {
        return (o1 * o2 < 0) && (o3 * o4 < 0);
    }
    
    if (o1 == 0 && o2 == 0 && o3 == 0 && o4 == 0) {
        //если все точки коллинеарны - проверяем перекрытие проекций
        double minX1 = std::min(p1.x(), p2.x());
        double maxX1 = std::max(p1.x(), p2.x());
        double minX2 = std::min(p3.x(), p4.x());
        double maxX2 = std::max(p3.x(), p4.x());
        
        double minY1 = std::min(p1.y(), p2.y());
        double maxY1 = std::max(p1.y(), p2.y());
        double minY2 = std::min(p3.y(), p4.y());
        double maxY2 = std::max(p3.y(), p4.y());
        
        return !(maxX1 < minX2 || maxX2 < minX1 || maxY1 < minY2 || maxY2 < minY1);
    }
    
    return false;
}

bool ConvexHullWidget::triangleDoesNotIntersectHull(const QPointF &pb, const QPointF &pe, 
                                                   const QPointF &pi, const QVector<QPointF> &hull)
{
    //если стороны треугольника не пересекаются с существующими сторонами оболочки
    for (int i = 0; i < hull.size(); ++i) {
        int next = (i + 1) % hull.size();
        
        //пропускаем сторону, которую мы заменяем
        if ((std::abs(hull[i].x() - pb.x()) < 1e-9 && std::abs(hull[i].y() - pb.y()) < 1e-9 &&
             std::abs(hull[next].x() - pe.x()) < 1e-9 && std::abs(hull[next].y() - pe.y()) < 1e-9) ||
            (std::abs(hull[i].x() - pe.x()) < 1e-9 && std::abs(hull[i].y() - pe.y()) < 1e-9 &&
             std::abs(hull[next].x() - pb.x()) < 1e-9 && std::abs(hull[next].y() - pb.y()) < 1e-9)) {
            continue;
        }
        
        //есть ли пересечение с новыми сторонами
        if (segmentsIntersect(pb, pi, hull[i], hull[next]) ||
            segmentsIntersect(pi, pe, hull[i], hull[next])) {
            return false;
        }
    }
    
    return true;
}

double ConvexHullWidget::triangleArea(const QPointF &p1, const QPointF &p2, const QPointF &p3)
{
    return std::abs((p2.x() - p1.x()) * (p3.y() - p1.y()) - (p3.x() - p1.x()) * (p2.y() - p1.y())) / 2.0;
}

double ConvexHullWidget::orientation(const QPointF &p, const QPointF &q, const QPointF &r)
{
    return (q.x() - p.x()) * (r.y() - p.y()) - (q.y() - p.y()) * (r.x() - p.x());
}

double ConvexHullWidget::distance(const QPointF &p1, const QPointF &p2)
{
    double dx = p2.x() - p1.x();
    double dy = p2.y() - p1.y();
    return dx * dx + dy * dy;
}

int ConvexHullWidget::findLowestPoint(const QVector<QPointF> &points)
{
    int lowestIndex = 0;
    for (int i = 1; i < points.size(); ++i) {
        if (points[i].y() < points[lowestIndex].y() || 
            (points[i].y() == points[lowestIndex].y() && points[i].x() < points[lowestIndex].x())) {
            lowestIndex = i;
        }
    }
    return lowestIndex;
}

void ConvexHullWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_points.isEmpty()) {
        painter.drawText(rect(), Qt::AlignCenter, "Загрузите файл с точками");
        return;
    }
    
    double minX = m_points[0].x(), maxX = m_points[0].x();
    double minY = m_points[0].y(), maxY = m_points[0].y();
    
    for (const QPointF &point : m_points) {
        minX = std::min(minX, point.x());
        maxX = std::max(maxX, point.x());
        minY = std::min(minY, point.y());
        maxY = std::max(maxY, point.y());
    }
    
    double margin = 50.0;
    double rangeX = maxX - minX;
    double rangeY = maxY - minY;
    
    if (rangeX < 1e-9) rangeX = 100.0;
    if (rangeY < 1e-9) rangeY = 100.0;
    
    double scaleX = (width() - 2 * margin) / rangeX;
    double scaleY = (height() - 2 * margin) / rangeY;
    double scale = std::min(scaleX, scaleY);
    
    auto transform = [&](const QPointF &p) -> QPointF {
        return QPointF(
            margin + (p.x() - minX) * scale,
            height() - margin - (p.y() - minY) * scale
        );
    };
    
    painter.setPen(QPen(Qt::gray, 1));
    QPointF origin = transform(QPointF(0, 0));
    painter.drawLine(0, origin.y(), width(), origin.y());
    painter.drawLine(origin.x(), 0, origin.x(), height());
    
    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(Qt::black);
    for (const QPointF &point : m_points) {
        QPointF transformed = transform(point);
        painter.drawEllipse(transformed, 1, 1); //размер точек 1пиксель
    }
    
    //рисуем выпуклую оболочку (пунктирная линия)
    if (m_convexHull.size() >= 3) {
        painter.setPen(QPen(Qt::gray, 2, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        
        QPolygonF convexPolygon;
        for (const QPointF &point : m_convexHull) {
            convexPolygon << transform(point);
        }
        
        painter.drawPolygon(convexPolygon);
    }
    
    //вогнутая оболочка (синяя)
    if (m_concaveHull.size() >= 3) {
        painter.setPen(QPen(Qt::blue, 3));
        painter.setBrush(Qt::NoBrush);
        
        QPolygonF concavePolygon;
        for (const QPointF &point : m_concaveHull) {
            concavePolygon << transform(point);
        }
        
        painter.drawPolygon(concavePolygon);
    }
    
    //вывод инфы
    QString info = QString("Точек: %1 | Выпуклая оболочка: %2 | Вогнутая оболочка: %3 | γ: %4")
                   .arg(m_points.size())
                   .arg(m_convexHull.size())
                   .arg(m_concaveHull.size())
                   .arg(m_gamma, 0, 'f', 2);
    painter.drawText(10, 20, info);
}
