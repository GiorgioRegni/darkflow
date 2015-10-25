#include <QStandardItemModel>
#include <QStandardItem>
#include <QFileDialog>

#include "filesselection.h"
#include "ui_filesselection.h"

FilesSelection::FilesSelection(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FilesSelection),
    m_list(new QStandardItemModel)
{
    ui->setupUi(this);
    ui->selectedFiles->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->selectedFiles->setModel(m_list);
}

FilesSelection::~FilesSelection()
{
    delete ui;
}

void FilesSelection::addClicked()
{
    QStringList list = QFileDialog::getOpenFileNames(this,
                                                     tr("Select files to insert"),
                                                     "/home/guigui",
                                                     "RAW Images (*.nef *.cr2 *.dng *.mef *.3fr *.raf *.x3f *.pef *.arw *.nrw);;"
                                                     "FITS Images (*.fits *.fit);;"
                                                     "TIFF Images (*.tif *.tiff);;"
                                                     "All Files (*.*)",
                                                     0, 0);
    foreach(QString file, list) {
        int pos = m_list->rowCount();
        QStandardItem *item = new QStandardItem(file);
        m_list->insertRow(pos);
        m_list->setItem(pos,item);
    }
}

void FilesSelection::removeClicked()
{
    ui->selectedFiles->setUpdatesEnabled(false);
    QModelIndexList indexes = ui->selectedFiles->selectionModel()->selectedIndexes();
    qSort(indexes);
    for (int i = indexes.count() - 1; i > -1; --i)
      m_list->removeRow(indexes.at(i).row());
    ui->selectedFiles->setUpdatesEnabled(true);
}

void FilesSelection::clearClicked()
{
    m_list->clear();
}

