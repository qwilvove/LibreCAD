#include "tt_dialogmain.h"
#include "ui_tt_dialogmain.h"

#include "document_interface.h"
#include "topographytools.h"
#include "tt_dialogedit.h"
#include "tt_dialogadd.h"

#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>

// Constructor
TT_DialogMain::TT_DialogMain(QWidget *parent, Document_Interface *doc) :
    QDialog(parent),
    ui(new Ui::TT_DialogMain),
    doc(doc)
{
    ui->setupUi(this);

    readSettings();

    if (!fileName.isEmpty())
    {
        ui->label->setText(tr("Active file : %1").arg(fileName));

        ui->pbSave->setEnabled(true);
        ui->pbImport->setEnabled(true);
        ui->pbAdd->setEnabled(true);
        ui->pbRemove->setEnabled(true);
        ui->pbEdit->setEnabled(true);
        ui->pbUp->setEnabled(true);
        ui->pbDown->setEnabled(true);
        ui->pbDraw->setEnabled(true);

        loadPoints();
        displayPoints();
    }
}

// Destructor
TT_DialogMain::~TT_DialogMain()
{
    delete ui;
}

// Read setting to find current .tt file
void TT_DialogMain::readSettings()
 {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "LibreCAD", "topographytools");
    fileName = settings.value("lastfilename", "").toString();
 }

// Save .tt file as current working file used
void TT_DialogMain::writeSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "LibreCAD", "topographytools");
    settings.setValue("lastfilename", fileName);
}

// Fill points list reading .tt file
// Data is stored as follows :
//   - Number of points
//   - Point 1 data
//   - Point 2 data
//   - ...
int TT_DialogMain::loadPoints()
{
    int nbPoints = 0;

    points.clear();

    QFile file(fileName);
    if (!file.exists())
    {
        QMessageBox::critical(this, tr("Error!"), tr("File does not exist!"));
        return -1;
    }
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, tr("Error!"), tr("Could not open file!"));
        return -1;
    }

    QDataStream stream(&file);
    if (stream.version() != QDataStream::Qt_6_0)
    {
        QMessageBox::critical(this, tr("Error!"), tr("Wrong data stream version!"));
        file.close();
        return -1;
    }

    int max;
    stream >> max;

    for (int i = 0; i < max; i++)
    {
        TT::Point point{};
        loadPoint(stream, point);
        points.append(point);
        nbPoints++;
    }

    file.close();

    return nbPoints;
}

// Read each information for a single point
void TT_DialogMain::loadPoint(QDataStream &stream, TT::Point &point)
{
    stream >> point.type;
    stream >> point.name;
    stream >> point.x;
    stream >> point.y;
    stream >> point.hasZ;
    stream >> point.z;
    stream >> point.ih;
    stream >> point.v0;
    stream >> point.ph;
    stream >> point.ha;
    stream >> point.va;
    stream >> point.id;
}

// Save all points in the .tt file following the file structure
// cf loadPoints for file structure
int TT_DialogMain::savePoints()
{
    int nbPoints = 0;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, tr("Error!"), tr("Could not save file!"));
        return -1;
    }

    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_6_0);

    int len = points.size();
    stream << len;

    foreach (TT::Point point, points)
    {
        savePoint(stream, point);
        nbPoints++;
    }

    file.close();

    return nbPoints;
}

// Write each information for a single point
void TT_DialogMain::savePoint(QDataStream &stream, TT::Point &point)
{
    stream << point.type;
    stream << point.name;
    stream << point.x;
    stream << point.y;
    stream << point.hasZ;
    stream << point.z;
    stream << point.ih;
    stream << point.v0;
    stream << point.ph;
    stream << point.ha;
    stream << point.va;
    stream << point.id;
}

// Read a CSV file to add points to the project
int TT_DialogMain::importPoints()
{
    int nbPoints = 0;

    QString fileNameLocal = QFileDialog::getOpenFileName(this, tr("Select a field notebook"), "", tr("CSV (*.csv)"));
    if (fileNameLocal.isEmpty())
    {
        return -1;
    }

    QFile file(fileNameLocal);
    if (!file.exists())
    {
        QMessageBox::warning(this, tr("Error!"), tr("File not found!"));
        return -1;
    }
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, tr("Error!"), tr("Could not open file!"));
        return -1;
    }

    QTextStream stream(&file);
    while (!stream.atEnd())
    {
        QString line = stream.readLine();
        TT::Point point{};
        if(importPoint(line, point))
        {
            points.append(point);
            nbPoints++;
        }
    }

    file.close();

    return nbPoints;
}

// Read point info on a single CSV line
// Accepted formats :
// xxx.xx;y.yyyy
// OR
// xx.xxxx;y.y;zz.zzz
bool TT_DialogMain::importPoint(QString &line, TT::Point &point)
{
    bool ok = true;

    QStringList splitedLine =  line.split(";");
    if (splitedLine.size() < 2)
    {
        return false;
    }

    if (splitedLine.size() == 2)
    {
        double x = splitedLine.at(0).toDouble(&ok);
        if (!ok) return false;
        double y = splitedLine.at(1).toDouble(&ok);
        if (!ok) return false;

        point.x = x;
        point.y = y;

        return true;
    }
    if (splitedLine.size() == 3)
    {
        double x = splitedLine.at(0).toDouble(&ok);
        if (!ok) return false;
        double y = splitedLine.at(1).toDouble(&ok);
        if (!ok) return false;
        double z = splitedLine.at(2).toDouble(&ok);
        if (!ok) return false;

        point.x = x;
        point.y = y;
        point.hasZ = true;
        point.z = z;

        return true;
    }

    return false;
}

// Update tableWidget to show point list to the user
void TT_DialogMain::displayPoints()
{
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);

    foreach (TT::Point point, points)
    {
        displayPoint(point);
    }
}

// Add a line to tableWidget and fill it with point info
void TT_DialogMain::displayPoint(TT::Point &point)
{
    // Add one line at the end
    int lineNumber = ui->tableWidget->rowCount() + 1;
    ui->tableWidget->setRowCount(lineNumber);

    // Fill the row
    // Check for type
    if (point.type == TT::PTYPE::POINT)
    {
        // Line number
        QString infoLineNumber = QString("%1").arg(lineNumber);
        QTableWidgetItem *itemLineNumber = new QTableWidgetItem(infoLineNumber);
        itemLineNumber->setForeground(QColor(QString("dark green")));
        ui->tableWidget->setItem(lineNumber - 1, 0, itemLineNumber);

        // Type
        QString infoType = tr("POINT");
        QTableWidgetItem *itemType = new QTableWidgetItem(infoType);
        itemType->setForeground(QColor(QString("dark green")));
        ui->tableWidget->setItem(lineNumber - 1, 1, itemType);

        // Name
        QString infoName = point.name;
        QTableWidgetItem *itemName = new QTableWidgetItem(infoName);
        itemName->setForeground(QColor(QString("dark green")));
        ui->tableWidget->setItem(lineNumber - 1, 2, itemName);

        // Parameters
        QString infoParameters = tr("X=%1 , Y=%2").arg(point.x, 11, 'f', 3).arg(point.y, 11, 'f', 3);
        if (point.hasZ)
        {
            infoParameters += tr(" , Z=%1").arg(point.z, 8, 'f', 3);
        }
        QTableWidgetItem *itemParameters = new QTableWidgetItem(infoParameters);
        itemParameters->setForeground(QColor(QString("dark green")));
        ui->tableWidget->setItem(lineNumber - 1, 3, itemParameters);
    }
    else if (point.type == TT::PTYPE::STATION)
    {
        // Line number
        QString infoLineNumber = QString("%1").arg(lineNumber);
        QTableWidgetItem *itemLineNumber = new QTableWidgetItem(infoLineNumber);
        itemLineNumber->setForeground(QColor(QString("dark red")));
        ui->tableWidget->setItem(lineNumber - 1, 0, itemLineNumber);

        // Type
        QString infoType = tr("STATION");
        QTableWidgetItem *itemType = new QTableWidgetItem(infoType);
        itemType->setForeground(QColor(QString("dark red")));
        ui->tableWidget->setItem(lineNumber - 1, 1, itemType);

        // Name
        QString infoName = point.name;
        QTableWidgetItem *itemName = new QTableWidgetItem(infoName);
        itemName->setForeground(QColor(QString("dark red")));
        ui->tableWidget->setItem(lineNumber - 1, 2, itemName);

        // Parameters
        QString infoParameters = tr("IH=%1").arg(point.ih, 7, 'f', 3);
        if (point.v0 >= 0)
        {
            infoParameters += tr(" , V0=%1").arg(point.v0, 8, 'f', 4);
        }
        QTableWidgetItem *itemParameters = new QTableWidgetItem(infoParameters);
        itemParameters->setForeground(QColor(QString("dark red")));
        ui->tableWidget->setItem(lineNumber - 1, 3, itemParameters);
    }
    else if (point.type == TT::PTYPE::REFERENCE)
    {
        // Line number
        QString infoLineNumber = QString("%1").arg(lineNumber);
        QTableWidgetItem *itemLineNumber = new QTableWidgetItem(infoLineNumber);
        itemLineNumber->setForeground(QColor(QString("dark blue")));
        ui->tableWidget->setItem(lineNumber - 1, 0, itemLineNumber);

        // Type
        QString infoType = tr("REFERENCE");
        QTableWidgetItem *itemType = new QTableWidgetItem(infoType);
        itemType->setForeground(QColor(QString("dark blue")));
        ui->tableWidget->setItem(lineNumber - 1, 1, itemType);

        // Name
        QString infoName = point.name;
        QTableWidgetItem *itemName = new QTableWidgetItem(infoName);
        itemName->setForeground(QColor(QString("dark blue")));
        ui->tableWidget->setItem(lineNumber - 1, 2, itemName);

        // Parameters
        QString infoParameters = tr("PH=%1 , HA=%2 , VA=%3 , ID=%4")
                .arg(point.ph, 7, 'f', 3)
                .arg(point.ha, 8, 'f', 4)
                .arg(point.va, 8, 'f', 4)
                .arg(point.id, 7, 'f', 3);
        QTableWidgetItem *itemParameters = new QTableWidgetItem(infoParameters);
        itemParameters->setForeground(QColor(QString("dark blue")));
        ui->tableWidget->setItem(lineNumber - 1, 3, itemParameters);
    }
    else // point.type == TT::PTYPE::MEASURE
    {
        // Line number
        QString infoLineNumber = QString("%1").arg(lineNumber);
        QTableWidgetItem *itemLineNumber = new QTableWidgetItem(infoLineNumber);
        itemLineNumber->setForeground(QColor(QString("dark cyan")));
        ui->tableWidget->setItem(lineNumber - 1, 0, itemLineNumber);

        // Type
        QString infoType = tr("MEASURE");
        QTableWidgetItem *itemType = new QTableWidgetItem(infoType);
        itemType->setForeground(QColor(QString("dark cyan")));
        ui->tableWidget->setItem(lineNumber - 1, 1, itemType);

        // Name
        QString infoName = point.name;
        QTableWidgetItem *itemName = new QTableWidgetItem(infoName);
        itemName->setForeground(QColor(QString("dark cyan")));
        ui->tableWidget->setItem(lineNumber - 1, 2, itemName);

        // Parameters
        QString infoParameters = tr("PH=%1 , HA=%2 , VA=%3 , ID=%4")
                .arg(point.ph, 7, 'f', 3)
                .arg(point.ha, 8, 'f', 4)
                .arg(point.va, 8, 'f', 4)
                .arg(point.id, 7, 'f', 3);
        QTableWidgetItem *itemParameters = new QTableWidgetItem(infoParameters);
        itemParameters->setForeground(QColor(QString("dark cyan")));
        ui->tableWidget->setItem(lineNumber - 1, 3, itemParameters);
    }
}

// Add a point to points
void TT_DialogMain::addPoint()
{
    TT::Point newPoint {};
    TT_DialogAdd addDialog(this, &newPoint);
    if (addDialog.exec() == QDialog::Accepted)
    {
        points.append(newPoint);
        displayPoints();
    }
}

// Remove points according to the list of indexes
void TT_DialogMain::removePoints(QList<int> indexesToRemove)
{
    // Sort the list (ex: 1 2 5 8 9 ...)
    std::sort(indexesToRemove.begin(), indexesToRemove.end());

    // Remove indexes from the end to the begin
    // in order to keep indexes correct
    for (int i = indexesToRemove.size() - 1; i >= 0; i--)
    {
        points.remove(indexesToRemove[i]);
    }

    displayPoints();
}

// Edit a single point attributes
void TT_DialogMain::editPoint(TT::Point &point)
{
    TT_DialogEdit editDialog(this, &point);
    if (editDialog.exec() == QDialog::Accepted)
    {
        displayPoints();
    }
}

// Move points[index] one position up in points QList
void TT_DialogMain::movePointUp(int index)
{
    points.move(index, index - 1);

    displayPoints();
}

// Move points[index] one position down in points QList
void TT_DialogMain::movePointDown(int index)
{
    points.move(index, index + 1);

    displayPoints();
}

// Draw points on the current drawing
int TT_DialogMain::drawPoints()
{
    int nbPoints = 0;

    // Prepare layers
    QString currentLayer = this->doc->getCurrentLayer();

    int colour;
    DPI::LineWidth lineWidth;
    DPI::LineType lineType;

    this->doc->setLayer("TT_POINTS");
    this->doc->getCurrentLayerProperties(&colour, &lineWidth, &lineType);
    colour = 0;
    this->doc->setCurrentLayerProperties(colour, lineWidth, lineType);

    this->doc->setLayer("TT_NAME");
    this->doc->getCurrentLayerProperties(&colour, &lineWidth, &lineType);
    colour = 0x00FFFF;
    this->doc->setCurrentLayerProperties(colour, lineWidth, lineType);

    this->doc->setLayer("TT_ALTI");
    this->doc->getCurrentLayerProperties(&colour, &lineWidth, &lineType);
    colour = 0x00FF00;
    this->doc->setCurrentLayerProperties(colour, lineWidth, lineType);

    // Draw each point
    for (auto i = 0; i < points.size(); i++)
    {
        TT::Point currentPoint = points.at(i);
        if (currentPoint.type == TT::PTYPE::POINT)
        {
            drawPoint(currentPoint);
            nbPoints++;
        }
    }

    this->doc->setLayer(currentLayer);

    return nbPoints;
}

// Draw a single point on the current drawing
void TT_DialogMain::drawPoint(TT::Point &point)
{
    this->doc->setLayer("TT_POINTS");
    QPointF insertionPoint(point.x, point.y);
    this->doc->addPoint(&insertionPoint);

    if (!point.name.isEmpty())
    {
        this->doc->setLayer("TT_NAME");
        QPointF textInsertionPoint(point.x + 1.0, point.y + 4.0);
        this->doc->addText(point.name, "standard", &textInsertionPoint, 12.0, 0.0, DPI::HAlignLeft, DPI::VAlignTop);
    }

    if (point.hasZ)
    {
        this->doc->setLayer("TT_ALTI");
        QString text = QString("%1").arg(point.z);
        QPointF textInsertionPoint(point.x + 1.0, point.y - 12.0 - 4.0);
        this->doc->addText(text, "standard", &textInsertionPoint, 12.0, 0.0, DPI::HAlignLeft, DPI::VAlignTop);
    }
}

void TT_DialogMain::on_pbNew_clicked()
{
    QString fileNameLocal = QFileDialog::getSaveFileName(this, tr("Create a TT file"), "", tr("TT files (*.tt)"));
    if (fileNameLocal.isEmpty())
    {
        return;
    }

    fileName = fileNameLocal;

    writeSettings();

    ui->label->setText(tr("Active file : %1").arg(fileName));

    ui->pbSave->setEnabled(true);
    ui->pbImport->setEnabled(true);
    ui->pbAdd->setEnabled(true);
    ui->pbRemove->setEnabled(true);
    ui->pbEdit->setEnabled(true);
    ui->pbUp->setEnabled(true);
    ui->pbDown->setEnabled(true);
    ui->pbDraw->setEnabled(true);

    points.clear();
    displayPoints();
}

void TT_DialogMain::on_pbOpen_clicked()
{
    QString fileNameLocal = QFileDialog::getOpenFileName(this, tr("Open a TT file"), "", tr("TT files (*.tt)"));
    if (fileNameLocal.isEmpty())
    {
        return;
    }

    QFile file(fileNameLocal);
    if (!file.exists())
    {
        return;
    }

    fileName = fileNameLocal;

    writeSettings();

    ui->label->setText(tr("Active file : %1").arg(fileName));

    ui->pbSave->setEnabled(true);
    ui->pbImport->setEnabled(true);
    ui->pbAdd->setEnabled(true);
    ui->pbRemove->setEnabled(true);
    ui->pbEdit->setEnabled(true);
    ui->pbUp->setEnabled(true);
    ui->pbDown->setEnabled(true);
    ui->pbDraw->setEnabled(true);

    loadPoints();
    displayPoints();
}

void TT_DialogMain::on_pbSave_clicked()
{
    int nbPointsSaved = TT_DialogMain::savePoints();
    if (nbPointsSaved > -1)
    {
        ui->label->setText(tr("Active file : %1 | %2 points saved.").arg(fileName).arg(nbPointsSaved));
    }
}

void TT_DialogMain::on_pbImport_clicked()
{
    int nbPointsImported = TT_DialogMain::importPoints();
    if (nbPointsImported > -1)
    {
        ui->label->setText(tr("Active file : %1 | %2 points imported.").arg(fileName).arg(nbPointsImported));
        displayPoints();
    }
}

void TT_DialogMain::on_pbAdd_clicked()
{
    addPoint();
}

void TT_DialogMain::on_pbRemove_clicked()
{
    // Only proceed if there are selected items
    if (!ui->tableWidget->selectedItems().isEmpty())
    {
        QList<int> indexesToRemove;

        // For each row
        for (int i = 0; i < ui->tableWidget->selectedItems().size(); i += ui->tableWidget->columnCount())
        {
            QTableWidgetItem *qtwi = ui->tableWidget->selectedItems().at(i);
            indexesToRemove.append(qtwi->row());
        }
        removePoints(indexesToRemove);
    }
}

void TT_DialogMain::on_pbEdit_clicked()
{
    if (ui->tableWidget->currentRow() >= 0 && ui->tableWidget->currentRow() < points.size())
    {
        editPoint(points[ui->tableWidget->currentRow()]);
    }
}

void TT_DialogMain::on_pbUp_clicked()
{
    if (ui->tableWidget->currentRow() >= 1 && ui->tableWidget->currentRow() < points.size())
    {
        movePointUp(ui->tableWidget->currentRow());
    }
}

void TT_DialogMain::on_pbDown_clicked()
{
    if (ui->tableWidget->currentRow() >= 0 && ui->tableWidget->currentRow() < points.size() - 1)
    {
        movePointDown(ui->tableWidget->currentRow());
    }
}

void TT_DialogMain::on_pbDraw_clicked()
{
    int nbPointsDrawn = TT_DialogMain::drawPoints();
    if (nbPointsDrawn > -1)
    {
        ui->label->setText(tr("Active file : %1 | %2 points drawn.").arg(fileName).arg(nbPointsDrawn));
    }
}

void TT_DialogMain::on_tableWidget_cellDoubleClicked(int row, int column)
{
    Q_UNUSED(column);

    editPoint(points[row]);
}



