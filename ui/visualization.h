#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include <QMainWindow>

namespace Ui {
class Visualization;
}

class Operator;
class OperatorOutput;
class Photo;

class Visualization : public QMainWindow
{
    Q_OBJECT
public:
    explicit Visualization(Operator *op, QWidget *parent = 0);
    ~Visualization();

signals:
    void operatorNameChanged(QString text);

public slots:
    void nameChanged(QString text);
    void updateTreeviewPhotos();
    void photoSelectionChanged();
    void zoomFitVisible();
    void zoomHalf();
    void zoomOne();
    void zoomDouble();
    void zoomCustom();
    void zoomPlus();
    void zoomMinus();
    void expChanged();
    void upToDate();
    void outOfDate();
    void playClicked();

    void histogramParamsChanged();
    void curveParamsChanged();

private:
    Ui::Visualization *ui;
    Operator *m_operator;
    OperatorOutput *m_output;
    Photo *m_photo;
    enum ZoomLevel {
        ZoomFitVisible,
        ZoomHalf,
        ZoomOne,
        ZoomDouble,
        ZoomCustom
    } m_zoomLevel;
    int m_zoom;
    QString m_currentPhoto;
    const OperatorOutput *m_currentOutput;

    void clearAllTabs();
    void updateTabs();
    void updateTabsWithPhoto();
    void updateTabsWithOutput();
    void updateVisualizationZoom();
    void updateTagsTable();

};

#endif // VISUALIZATION_H
