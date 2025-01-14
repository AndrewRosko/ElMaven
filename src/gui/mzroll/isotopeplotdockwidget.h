#ifndef ISOTOPEPLOTDOCKWIDGET_H
#define ISOTOPEPLOTDOCKWIDGET_H

#include <QDockWidget>

class MainWindow;

namespace Ui {
class IsotopePlotDockWidget;
}

class IsotopePlotDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit IsotopePlotDockWidget(MainWindow *mw = 0);
    ~IsotopePlotDockWidget();

private Q_SLOTS:
    void updateC13Flag(bool setState);
    void updateN15Flag(bool setState);
    void updateD2Flag(bool setState);
    void updateS34Flag(bool setState);
    void setPoolThreshold(double poolThreshold);

private:
    Ui::IsotopePlotDockWidget *ui;
    MainWindow *_mw;
    QCheckBox* _C13;
    QCheckBox* _D2;
    QCheckBox* _N15;
    QCheckBox* _S34;

    void setToolBar();
    void recompute();
};

#endif // ISOTOPEPLOTDOCKWIDGET_H
