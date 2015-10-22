#ifndef PROCESS_H
#define PROCESS_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QSet>
#include <QPointF>
#include <QPoint>

class ProcessScene;
class QGraphicsSceneContextMenuEvent;
class QEvent;
class Operator;
class ProcessNode;
class ProcessConnection;
class QGraphicsItem;


class Process : public QObject
{
    Q_OBJECT
public:
    explicit Process(ProcessScene *scene, QObject *parent = 0);
    ~Process();

    QString projectName() const;
    void setProjectName(const QString &projectName);

    QString projectFile() const;
    void setProjectFile(const QString &projectFile);

    QString notes() const;
    void setNotes(const QString &notes);

    QString outputDirectory() const;
    void setOutputDirectory(const QString &outputDirectory);

    QString temporaryDirectory() const;
    void setTemporaryDirectory(const QString &temporaryDirectory);

    void reset();
    void save();
    void open(const QString& filename);


    bool dirty() const;
    void setDirty(bool dirty);

    void addOperator(Operator *op);
    void spawnContextMenu(const QPoint& pos);

signals:
    void stateChanged();

public slots:

private slots:
    void contextMenuSignal(QGraphicsSceneContextMenuEvent *);

private:
    Q_DISABLE_COPY(Process);
    QString m_projectName;
    QString m_projectFile;
    QString m_notes;
    QString m_outputDirectory;
    QString m_temporaryDirectory;
    ProcessScene *m_scene;
    bool m_dirty;
    QVector<Operator*> m_availableOperators;
    QPointF m_lastMousePosition;
    QSet<ProcessNode*> m_nodes;
    ProcessConnection *m_conn;


    QGraphicsItem* findItem(const QPointF &pos, int type);
    void resetAllButtonsBut(QGraphicsItem*item=0);
    bool eventFilter(QObject *obj, QEvent *event);
};

#endif // PROCESS_H
