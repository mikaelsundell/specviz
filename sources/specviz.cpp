// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#include "specviz.h"
#include "icctransform.h"
#include "platform.h"
#include "qcustomplot/qcustomplot.h"
#include "question.h"
#include "specio.h"
#include "stylesheet.h"
#include <QActionGroup>
#include <QClipboard>
#include <QColorDialog>
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QMimeData>
#include <QObject>
#include <QPointer>
#include <QSettings>
#include <QToolButton>

// generated files
#include "ui_about.h"
#include "ui_specviz.h"

class SpecvizPrivate : public QObject {
    Q_OBJECT
public:
    SpecvizPrivate();
    void init();
    void initPlot();
    bool loadDataset(const QString& filename);
    QCustomPlot* plot();
    QTreeWidget* header();
    QTreeWidget* tree();
    QVariant settingsValue(const QString& key, const QVariant& defaultValue = QVariant());
    void setSettingsValue(const QString& key, const QVariant& value);
    bool eventFilter(QObject* object, QEvent* event);
    void enable(bool enable);
    void profile();
    void stylesheet();

public Q_SLOTS:
    void open();
    void exportSelected();
    void copyImage();
    void clear();
    void openAbout();
    void openGithubReadme();
    void openGithubIssues();
    void updatePlot();
    void plotmouseMoveEvent(QMouseEvent* event);
    void itemChanged(QTreeWidgetItem* item, int column);
    void itemSelectionChanged();

public:
    class About : public QDialog {
    public:
        About(QWidget* parent = nullptr)
            : QDialog(parent)
        {
            QScopedPointer<Ui_About> about;
            about.reset(new Ui_About());
            about->setupUi(this);
            about->name->setText(PROJECT_NAME);
            about->version->setText(PROJECT_VERSION);
            about->copyright->setText(PROJECT_COPYRIGHT);
            QString url = GITHUB_URL;
            about->github->setText(QString("Github project: <a href='%1'>%1</a>").arg(url));
            about->github->setTextFormat(Qt::RichText);
            about->github->setTextInteractionFlags(Qt::TextBrowserInteraction);
            about->github->setOpenExternalLinks(true);
            QFile file(":/files/resources/Copyright.txt");
            file.open(QIODevice::ReadOnly | QIODevice::Text);
            QTextStream in(&file);
            QString text = in.readAll();
            file.close();
            about->licenses->setText(text);
        }
    };
    struct Data {
        QStringList arguments;
        QStringList extensions;
        QVector<QPointer<QCPItemTracer>> tracers;
        QList<SpecFile::Dataset> datasets;
        QPointer<QCPItemRect> gradientRect;
        QScopedPointer<About> about;
        QScopedPointer<Ui_Specviz> ui;
        QPointer<Specviz> window;
    };
    Data d;
};

SpecvizPrivate::SpecvizPrivate() { d.extensions = SpecIO::availableExtensions(); }

void
SpecvizPrivate::init()
{
    platform::setDarkTheme();
    // icc profile
    ICCTransform* transform = ICCTransform::instance();
    QDir resources(platform::getApplicationPath() + "/Resources");
    QString inputProfile = resources.filePath("sRGB2014.icc");  // built-in Qt input profile
    transform->setInputProfile(inputProfile);
    profile();
    // about
    d.about.reset(new About(d.window.data()));
    // ui
    d.ui.reset(new Ui_Specviz());
    d.ui->setupUi(d.window.data());
    initPlot();
    // tree
    tree()->setHeaderLabels(QStringList() << "Dataset"
                                          << "Display"
                                          << "Source");
    tree()->setColumnWidth(0, 160);
    tree()->setColumnWidth(1, 100);
    tree()->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    // header
    header()->setHeaderLabels(QStringList() << "Name"
                                            << "Value");
    header()->setColumnWidth(0, 160);
    header()->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    // connect
    connect(d.ui->fileOpen, &QAction::triggered, this, &SpecvizPrivate::open);
    connect(d.ui->fileExportSelected, &QAction::triggered, this, &SpecvizPrivate::exportSelected);
    connect(d.ui->editCopyImage, &QAction::triggered, this, &SpecvizPrivate::copyImage);
    connect(d.ui->editClear, &QAction::triggered, this, &SpecvizPrivate::clear);
    connect(d.ui->helpAbout, &QAction::triggered, this, &SpecvizPrivate::openAbout);
    connect(d.ui->helpGithubReadme, &QAction::triggered, this, &SpecvizPrivate::openGithubReadme);
    connect(d.ui->helpGithubIssues, &QAction::triggered, this, &SpecvizPrivate::openGithubIssues);
    connect(d.ui->plotWidget, &QCustomPlot::afterReplot, this, &SpecvizPrivate::updatePlot);
    connect(d.ui->plotWidget, &QCustomPlot::mouseMove, this, &SpecvizPrivate::plotmouseMoveEvent);
    connect(d.ui->treeWidget, &QTreeWidget::itemChanged, this, &SpecvizPrivate::itemChanged);
    connect(d.ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &SpecvizPrivate::itemSelectionChanged);
    // stylesheet
    stylesheet();
// debug
#ifdef QT_DEBUG
    QMenu* menu = d.ui->menubar->addMenu("Debug");
    {
        QAction* action = new QAction("Reload stylesheet...", this);
        action->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S));
        menu->addAction(action);
        connect(action, &QAction::triggered, [&]() { this->stylesheet(); });
    }
#endif
    enable(false);
}

void
SpecvizPrivate::initPlot()
{
    if (d.gradientRect) {
        d.ui->plotWidget->removeItem(d.gradientRect);
        d.gradientRect.clear();
    }

    d.ui->plotWidget->xAxis->setRange(0.0, 1.0);
    d.ui->plotWidget->yAxis->setRange(0.0, 1.0);
    d.ui->plotWidget->setMouseTracking(true);
    d.ui->plotWidget->installEventFilter(this);
    d.ui->dataWidget->setVisible(false);

    d.gradientRect = new QCPItemRect(d.ui->plotWidget);
    d.gradientRect->topLeft->setType(QCPItemPosition::ptPlotCoords);
    d.gradientRect->bottomRight->setType(QCPItemPosition::ptPlotCoords);
    d.gradientRect->topLeft->setCoords(380, 0);
    d.gradientRect->bottomRight->setCoords(780, 0);

    QLinearGradient grad(0, 0, 1, 0);
    grad.setCoordinateMode(QGradient::ObjectBoundingMode);
    grad.setColorAt(0.0, QColor::fromHslF(0.72, 1.0, 0.5));
    grad.setColorAt(0.15, QColor::fromHslF(0.66, 1.0, 0.5));
    grad.setColorAt(0.3, QColor::fromHslF(0.5, 1.0, 0.5));
    grad.setColorAt(0.55, QColor::fromHslF(0.17, 1.0, 0.5));
    grad.setColorAt(0.75, QColor::fromHslF(0.0, 1.0, 0.5));
    grad.setColorAt(1.0, QColor::fromHslF(0.0, 1.0, 0.2));

    d.gradientRect->setBrush(QBrush(grad));
    d.gradientRect->setPen(Qt::NoPen);

    updatePlot();
}

bool
SpecvizPrivate::loadDataset(const QString& filename)
{
    SpecIO spec(filename);
    if (!spec.isLoaded()) {
        return false;
    }

    auto ds = spec.data();
    d.datasets.append(ds);
    QTreeWidgetItem* treeItem = new QTreeWidgetItem(tree());
    treeItem->setText(0, ds.name);

    QComboBox* combo = new QComboBox(tree());
    combo->addItems({ "Solid", "Dash", "Dot", "Dash dot", "Dash dot dot" });
    tree()->setItemWidget(treeItem, 1, combo);

    treeItem->setText(2, QFileInfo(filename).fileName());
    treeItem->setCheckState(0, Qt::Checked);
    treeItem->setData(0, Qt::UserRole, QVariant::fromValue(d.datasets.size() - 1));

    QVector<int> graphIndices;
    for (int i = 0; i < ds.indices.size(); ++i) {
        d.ui->plotWidget->addGraph();
        int graphIndex = d.ui->plotWidget->graphCount() - 1;
        QCPGraph* graph = d.ui->plotWidget->graph(graphIndex);
        graph->setName(ds.indices[i]);

        QColor color;
        QString idx = ds.indices[i].toUpper();
        if (idx == "R")
            color = Qt::red;
        else if (idx == "G")
            color = Qt::green;
        else if (idx == "B")
            color = Qt::blue;
        else
            color = QColor::fromHslF((i * 0.15), 0.7, 0.5);

        graph->setPen(QPen(color, 2));

        QVector<double> x, y;
        x.reserve(ds.data.size());
        y.reserve(ds.data.size());
        for (auto it = ds.data.begin(); it != ds.data.end(); ++it) {
            x << it.key();
            y << (i < it.value().size() ? it.value().at(i) : 0.0);
        }
        graph->setData(x, y);

        QTreeWidgetItem* child = new QTreeWidgetItem(treeItem);
        child->setText(0, ds.indices[i]);
        child->setCheckState(0, Qt::Checked);
        child->setData(0, Qt::UserRole, graphIndex);
        graph->setVisible(true);

        QComboBox* colorCombo = new QComboBox(tree());
        QList<QPair<QColor, QString>> colors
            = { { Qt::red, "Red" },         { Qt::green, "Green" },   { Qt::blue, "Blue" },   { Qt::cyan, "Cyan" },
                { Qt::magenta, "Magenta" }, { Qt::yellow, "Yellow" }, { Qt::black, "Black" }, { Qt::gray, "Gray" } };

        for (const auto& entry : colors) {
            QPixmap swatch(16, 16);
            swatch.fill(entry.first);
            colorCombo->addItem(QIcon(swatch), entry.second, entry.first);
        }

        int idxColor = -1;
        for (int i = 0; i < colors.size(); ++i) {
            if (colors[i].first == color) {
                idxColor = i;
                break;
            }
        }
        if (idxColor >= 0) {
            colorCombo->setCurrentIndex(idxColor);
        }

        connect(colorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
            QColor c = colorCombo->itemData(index).value<QColor>();
            QPen pen = d.ui->plotWidget->graph(graphIndex)->pen();
            pen.setColor(c);
            d.ui->plotWidget->graph(graphIndex)->setPen(pen);
            updatePlot();
        });

        tree()->setItemWidget(child, 1, colorCombo);

        graphIndices << graphIndex;

        QCPItemTracer* tracer = new QCPItemTracer(d.ui->plotWidget);
        tracer->setGraph(graph);
        tracer->setInterpolating(true);
        tracer->setStyle(QCPItemTracer::tsCircle);
        tracer->setPen(QPen(Qt::black));
        tracer->setBrush(Qt::yellow);
        tracer->setSize(10);
        tracer->setVisible(false);
        tracer->setLayer("overlay");

        d.tracers.append(tracer);
    }

    tree()->expandItem(treeItem);
    tree()->setCurrentItem(treeItem);

    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
        Qt::PenStyle style = Qt::SolidLine;
        switch (index) {
        case 1: style = Qt::DashLine; break;
        case 2: style = Qt::DotLine; break;
        case 3: style = Qt::DashDotLine; break;
        case 4: style = Qt::DashDotDotLine; break;
        default: style = Qt::SolidLine; break;
        }
        for (int gi : graphIndices) {
            QPen pen = d.ui->plotWidget->graph(gi)->pen();
            pen.setStyle(style);
            d.ui->plotWidget->graph(gi)->setPen(pen);
        }
        updatePlot();
    });

    enable(true);

    return true;
}


QTreeWidget*
SpecvizPrivate::header()
{
    return d.ui->headerWidget;
}

QTreeWidget*
SpecvizPrivate::tree()
{
    return d.ui->treeWidget;
}

QVariant
SpecvizPrivate::settingsValue(const QString& key, const QVariant& defaultValue)
{
    QSettings settings(PROJECT_IDENTIFIER, PROJECT_NAME);
    return settings.value(key, defaultValue);
}

void
SpecvizPrivate::setSettingsValue(const QString& key, const QVariant& value)
{
    QSettings settings(PROJECT_IDENTIFIER, PROJECT_NAME);
    settings.setValue(key, value);
}

bool
SpecvizPrivate::eventFilter(QObject* object, QEvent* event)
{
    if (object == d.ui->plotWidget && event->type() == QEvent::Leave) {
        for (auto& tracer : d.tracers)
            if (tracer)
                tracer->setVisible(false);
        d.ui->plotWidget->replot(QCustomPlot::rpQueuedReplot);
    }
    if (event->type() == QEvent::ScreenChangeInternal) {
        profile();
        stylesheet();
    }
    return QObject::eventFilter(object, event);
}

void
SpecvizPrivate::enable(bool enable)
{
    d.ui->dataWidget->setVisible(enable);
    d.ui->fileExportSelected->setEnabled(enable);
}

void
SpecvizPrivate::profile()
{
    QString outputProfile = platform::getIccProfileUrl(d.window->winId());
    // icc profile
    ICCTransform* transform = ICCTransform::instance();
    transform->setOutputProfile(outputProfile);
}

void
SpecvizPrivate::stylesheet()
{
    QString path = platform::getApplicationPath() + "/Resources/App.qss";
    auto ss = Stylesheet::instance();
    if (ss->loadQss(path)) {
        ss->applyQss(ss->compiled());
    }
    // qcustomplot
    QColor base = ss->color(Stylesheet::Base);
    d.ui->plotWidget->setBackground(QBrush(base));
    d.ui->plotWidget->axisRect()->setBackground(QBrush(base));

    QColor text = ss->color(Stylesheet::Text);
    QPen axisPen(text);
    d.ui->plotWidget->xAxis->setBasePen(axisPen);
    d.ui->plotWidget->yAxis->setBasePen(axisPen);
    d.ui->plotWidget->xAxis->setTickPen(axisPen);
    d.ui->plotWidget->yAxis->setTickPen(axisPen);
    d.ui->plotWidget->xAxis->setSubTickPen(axisPen);
    d.ui->plotWidget->yAxis->setSubTickPen(axisPen);
    d.ui->plotWidget->xAxis->setTickLabelColor(text);
    d.ui->plotWidget->yAxis->setTickLabelColor(text);

    QFont labelFont = d.ui->plotWidget->xAxis->labelFont();
    labelFont.setPointSize(11);
    d.ui->plotWidget->xAxis->setLabelFont(labelFont);
    d.ui->plotWidget->yAxis->setLabelFont(labelFont);
    d.ui->plotWidget->xAxis->setLabelColor(text);
    d.ui->plotWidget->yAxis->setLabelColor(text);

    QColor grid = ss->color(Stylesheet::Border);
    QPen gridPen(grid);
    d.ui->plotWidget->xAxis->grid()->setPen(gridPen);
    d.ui->plotWidget->yAxis->grid()->setPen(gridPen);
    d.ui->plotWidget->legend->setBrush(QBrush(base));

    QColor border = ss->color(Stylesheet::Border);
    d.ui->plotWidget->legend->setBorderPen(QPen(border));

    QFont legendFont = d.ui->plotWidget->legend->font();
    legendFont.setPointSize(11);
    d.ui->plotWidget->legend->setFont(legendFont);
    d.ui->plotWidget->legend->setTextColor(text);
    d.ui->plotWidget->legend->setIconBorderPen(QPen(border));
}

void
SpecvizPrivate::open()
{
    QString openDir = settingsValue("openDir", QDir::homePath()).toString();
    QStringList filters;
    for (const QString& ext : d.extensions) {
        filters.append("*." + ext);
    }
    QString filter = QString("Spectral data files (%1)").arg(filters.join(' '));
    QString filename = QFileDialog::getOpenFileName(d.window.data(), "Open spectral data file", openDir, filter);
    if (loadDataset(filename)) {
        setSettingsValue("openDir", QFileInfo(filename).absolutePath());
    }
}

void
SpecvizPrivate::exportSelected()
{
    QTreeWidgetItem* rootItem = d.ui->treeWidget->currentItem();
    if (!rootItem) {
        return;
    }
    while (rootItem->parent()) {
        rootItem = rootItem->parent();
    }

    int datasetIndex = rootItem->data(0, Qt::UserRole).toInt();
    if (datasetIndex < 0 || datasetIndex >= d.datasets.size()) {
        return;
    }
    const SpecFile::Dataset& ds = d.datasets[datasetIndex];

    QStringList filters;
    for (const QString& ext : d.extensions) {
        filters.append(QString("*.%1").arg(ext));
    }
    QString filter = QString("Spectral data files (%1)").arg(filters.join(' '));
    QString saveDir = settingsValue("saveDir", QDir::homePath()).toString();
    QString filename = QFileDialog::getSaveFileName(d.window.data(), tr("Export spectral dataset"), saveDir, filter);

    if (filename.isEmpty())
        return;

    setSettingsValue("saveDir", QFileInfo(filename).absolutePath());

    bool success = SpecIO::write(ds, filename);

    if (!success) {
        qWarning() << "failed to export dataset to:" << filename;
    }
}

void
SpecvizPrivate::copyImage()
{
    if (!d.ui || !d.ui->plotWidget)
        return;

    QSize size = d.ui->plotWidget->size();
    QPixmap pixmap = d.ui->plotWidget->toPixmap(size.width(), size.height());
    if (!pixmap.isNull()) {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setPixmap(pixmap);
    }
}

void
SpecvizPrivate::clear()
{
    if (d.datasets.isEmpty()) {
        return;
    }

    if (Question::askQuestion(d.window, "Are you sure you want to remove all datasets and clear the plot?")) {
        QSignalBlocker blockTree(d.ui->treeWidget);
        QSignalBlocker blockHeader(d.ui->headerWidget);

        d.datasets.clear();
        tree()->clear();
        header()->clear();
        d.ui->plotWidget->clearGraphs();
        d.ui->plotWidget->legend->setVisible(false);
        d.ui->plotWidget->xAxis->setLabel("");
        d.ui->plotWidget->yAxis->setLabel("");
        initPlot();

        enable(false);
    }
}

void
SpecvizPrivate::openAbout()
{
    d.about->exec();
}

void
SpecvizPrivate::openGithubReadme()
{
    QDesktopServices::openUrl(QUrl(QString("%1/blob/master/README.md").arg(GITHUB_URL)));
}

void
SpecvizPrivate::openGithubIssues()
{
    QDesktopServices::openUrl(QUrl(QString("%1/issues").arg(GITHUB_URL)));
}

void
SpecvizPrivate::updatePlot()
{
    QCPRange yr = d.ui->plotWidget->yAxis->range();
    double yMin = yr.lower;
    double yMax = yr.upper;
    double height = (yMax - yMin) * 0.01;

    d.gradientRect->topLeft->setCoords(380, yMin);
    d.gradientRect->bottomRight->setCoords(780, yMin + height);
    d.ui->plotWidget->replot();
}

void
SpecvizPrivate::plotmouseMoveEvent(QMouseEvent* event)
{
    double x = d.ui->plotWidget->xAxis->pixelToCoord(event->pos().x());
    QString message;

    QTreeWidgetItem* rootItem = d.ui->treeWidget->currentItem();
    while (rootItem && rootItem->parent()) {
        rootItem = rootItem->parent();
    }
    if (rootItem) {
        int datasetIndex = rootItem->data(0, Qt::UserRole).toInt();
        if (datasetIndex >= 0 && datasetIndex < d.datasets.size()) {
            message = d.datasets[datasetIndex].name;
        }
    }
    if (d.ui->trace->isChecked()) {
        QString traceMsg;
        for (int i = 0; i < d.ui->plotWidget->graphCount(); ++i) {
            QCPGraph* graph = d.ui->plotWidget->graph(i);
            if (!graph->visible() || i >= d.tracers.size())
                continue;

            QCPItemTracer* tracer = d.tracers[i];
            if (!tracer)
                continue;

            tracer->setGraph(graph);
            tracer->setGraphKey(x);
            tracer->setVisible(true);

            double y = tracer->position->value();
            traceMsg += QString("  %1: %2, %3").arg(graph->name()).arg(x, 0, 'f', 2).arg(y, 0, 'f', 3);
        }
        if (!traceMsg.isEmpty()) {
            message += " " + traceMsg;
        }
    }
    d.ui->plotWidget->replot(QCustomPlot::rpQueuedReplot);
    d.ui->dataset->setText(message.trimmed());
}

void
SpecvizPrivate::itemChanged(QTreeWidgetItem* item, int column)
{
    if (!item->parent()) {
        Qt::CheckState rootState = item->checkState(0);
        for (int i = 0; i < item->childCount(); ++i) {
            QTreeWidgetItem* child = item->child(i);
            child->setCheckState(0, rootState);
        }
    }
    else {
        bool visible = (item->checkState(0) == Qt::Checked);
        int graphIndex = item->data(0, Qt::UserRole).toInt();
        if (graphIndex >= 0 && graphIndex < d.ui->plotWidget->graphCount()) {
            d.ui->plotWidget->graph(graphIndex)->setVisible(visible);
        }
    }
    updatePlot();
}

void
SpecvizPrivate::itemSelectionChanged()
{
    QTreeWidgetItem* rootItem = d.ui->treeWidget->currentItem();
    while (rootItem->parent()) {
        rootItem = rootItem->parent();
    }

    int datasetIndex = rootItem->data(0, Qt::UserRole).toInt();
    const auto& ds = d.datasets[datasetIndex];

    header()->clear();
    QTreeWidgetItem* headerItem = new QTreeWidgetItem(header());
    headerItem->setText(0, "header");
    headerItem->setFlags(headerItem->flags() & ~Qt::ItemIsUserCheckable);

    for (auto it = ds.header.constBegin(); it != ds.header.constEnd(); ++it) {
        QTreeWidgetItem* meta = new QTreeWidgetItem(headerItem);
        meta->setText(0, it.key());
        meta->setText(1, it.value().toString());
        meta->setFlags(meta->flags() & ~Qt::ItemIsUserCheckable);
    }

    header()->expandItem(headerItem);

    d.ui->plotWidget->legend->setVisible(true);
    d.ui->plotWidget->xAxis->setLabel("wavelength (nm)");
    d.ui->plotWidget->yAxis->setLabel(ds.units + " (selected)");
    d.ui->plotWidget->rescaleAxes();
    updatePlot();

    d.ui->dataset->setText(ds.name);
}

#include "specviz.moc"

Specviz::Specviz(QWidget* parent)
    : QMainWindow(parent)
    , p(new SpecvizPrivate())
{
    p->d.window = this;
    p->init();
}

Specviz::~Specviz() {}

void
Specviz::setArguments(const QStringList& arguments)
{
    p->d.arguments = arguments;
    for (int i = 0; i < arguments.size(); ++i) {
        if (arguments[i] == "--open" && i + 1 < arguments.size()) {
            QString filename = arguments[i + 1];
            if (!filename.isEmpty()) {
                QFileInfo fileInfo(filename);
                if (p->d.extensions.contains(fileInfo.suffix().toLower())) {
                    if (p->loadDataset(filename)) {
                        p->setSettingsValue("openDir", QFileInfo(filename).absolutePath());
                    }
                    else {
                        qWarning() << "Could not load dataset from filename: " << filename;
                    }
                    return;
                }
            }
        }
    }
}

void
Specviz::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl& url : urls) {
            QString filename = url.toLocalFile();
            QString extension = QFileInfo(filename).suffix().toLower();
            if (p->d.extensions.contains(extension)) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void
Specviz::dropEvent(QDropEvent* event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl& url : urls) {
        QString filename = url.toLocalFile();
        QString extension = QFileInfo(filename).suffix().toLower();
        if (p->d.extensions.contains(extension)) {
            if (p->loadDataset(filename)) {
                p->setSettingsValue("openDir", QFileInfo(filename).absolutePath());
            }
        }
    }
}
