#ifndef TINYPLOT_H
#define TINYPLOT_H

#include "stable.h"

class EIC;

class TinyPlot: public QObject, public QGraphicsItem {
    Q_OBJECT

#if QT_VERSION >= 0x040600
    Q_INTERFACES( QGraphicsItem )
#endif

public:
    TinyPlot(QGraphicsItem *parent = nullptr, QGraphicsScene *scene = nullptr);
    void addData(EIC* eic,
                 float rtMin,
                 float rtMax,
                 bool highlightRange = false,
                 float peakRtMin = -1.0f,
                 float peakRtMax = -1.0f);
    void setColor(QColor color) { _color = color; }
    void clearData();
	void setWidth(int w)  { _width=w; }
	void setHeight(int h) { _height=h; }
    pair<float, float> xBounds() { return {_minXValue, _maxXValue}; }
    void setXBounds(float x1, float x2) { _minXValue = x1; _maxXValue = x2; }
    pair<float, float> yBounds() { return {_minYValue, _maxYValue}; }
    void setYBounds(float y1, float y2) { _minYValue = y1; _maxYValue = y2; }
    void computeXBoundsFromData();
    void computeYBoundsFromData();
    void setDrawAxes(bool draw) { _drawAxes = draw; }
    bool drawAxes() { return _drawAxes; }
    void setAxesOffset(float offset) { _axesOffset = offset; }
    QPointF mapToPlot(float x, float y);
    void setDrawNoPeakMessages(bool draw) { _drawNoPeakMessages = draw; }
    void setNoPeakMessage(QString message) { _noPeakMessage = message; }
    void setNoPeakSubMessage(QString message) { _noPeakSubMessage = message; }

protected:
    QRectF boundingRect() const;
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget);

private:
    struct PlotData {
        QVector<QPointF> leftRegion;
        QVector<QPointF> peakRegion;
        QVector<QPointF> rightRegion;
        QVector<QPointF> baseline;

        void clear() {
            leftRegion.clear();
            peakRegion.clear();
            rightRegion.clear();
            baseline.clear();
        }
    };

    void _addAxes(QPainter* painter);

	int _width;
	int _height;
    float _axesOffset;
    bool _drawAxes;

    bool _noPeakData;
    float _minXValue, _minYValue, _maxXValue, _maxYValue;
    PlotData _data;
    QColor _color;
    bool _drawNoPeakMessages;
    QString _noPeakMessage;
    QString _noPeakSubMessage;
};

#endif
