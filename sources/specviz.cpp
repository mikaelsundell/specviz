// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#include "specviz.h"
#include "icctransform.h"
#include "platform.h"
#include "qcustomplot/qcustomplot.h"
#include "specfile.h"
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
    void itemChanged(QTreeWidgetItem* item, int column);
    void itemSelectionChanged();
    void clear();
    void openGithubReadme();
    void openGithubIssues();

public:
    struct Data {
        QStringList arguments;
        QStringList extensions;
        QList<SpecReader::Dataset> datasets;
        QScopedPointer<Ui_Specviz> ui;
        QPointer<Specviz> window;
    };
    Data d;
};

SpecvizPrivate::SpecvizPrivate() { d.extensions = { "json" }; }

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
}

void
SpecvizPrivate::initPlot()
{
    QCPItemRect *gradientRect = new QCPItemRect(d.ui->plotWidget);
    gradientRect->topLeft->setType(QCPItemPosition::ptPlotCoords);
    gradientRect->bottomRight->setType(QCPItemPosition::ptPlotCoords);
    gradientRect->topLeft->setCoords(380, 0);
    gradientRect->bottomRight->setCoords(780, 0.01);

    QLinearGradient grad(0, 0, 1, 0);
    grad.setCoordinateMode(QGradient::ObjectBoundingMode);

    // approximate spectral hues
    grad.setColorAt(0.0, QColor::fromHslF(0.72, 1.0, 0.5));
    grad.setColorAt(0.15, QColor::fromHslF(0.66, 1.0, 0.5));
    grad.setColorAt(0.3, QColor::fromHslF(0.5, 1.0, 0.5));
    grad.setColorAt(0.55, QColor::fromHslF(0.17, 1.0, 0.5));
    grad.setColorAt(0.75, QColor::fromHslF(0.0, 1.0, 0.5));
    grad.setColorAt(1.0, QColor::fromHslF(0.0, 1.0, 0.2));

    gradientRect->setBrush(QBrush(grad));
    gradientRect->setPen(Qt::NoPen);
}


bool
SpecvizPrivate::loadDataset(const QString& filename)
{
    SpecFile spec(filename);
    if (!spec.isLoaded()) {
        return false;
    }

    auto ds = spec.data();
    d.datasets.append(ds);
    QTreeWidgetItem* treeItem = new QTreeWidgetItem(tree());
    treeItem->setText(0, ds.header.value("model").toString());
    
    QComboBox *combo = new QComboBox(tree());
    combo->addItems({"Solid", "Dash", "Dot", "Dash dot", "Dash dot dot"});
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

        // default colors
        QString idx = ds.indices[i].toUpper();
        if (idx == "R")      graph->setPen(QPen(Qt::red, 2));
        else if (idx == "G") graph->setPen(QPen(Qt::green, 2));
        else if (idx == "B") graph->setPen(QPen(Qt::blue, 2));
        else                 graph->setPen(QPen(QColor::fromHslF((i * 0.15), 0.7, 0.5), 2));

        // set graph data
        QVector<double> x, y;
        x.reserve(ds.data.size());
        y.reserve(ds.data.size());
        for (auto it = ds.data.begin(); it != ds.data.end(); ++it) {
            x << it.key();
            y << (i < it.value().size() ? it.value().at(i) : 0.0);
        }
        graph->setData(x, y);

        // child tree item
        QTreeWidgetItem* child = new QTreeWidgetItem(treeItem);
        child->setText(0, ds.indices[i]);
        child->setCheckState(0, Qt::Checked);
        child->setData(0, Qt::UserRole, graphIndex);
        d.ui->plotWidget->graph(graphIndex)->setVisible(true);

        graphIndices << graphIndex;
    }
    tree()->expandItem(treeItem);
    tree()->setCurrentItem(treeItem);
    
    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        [=](int index) {
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
            d.ui->plotWidget->replot();
        });

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
    if (event->type() == QEvent::ScreenChangeInternal) {
        profile();
        stylesheet();
    }
    return QObject::eventFilter(object, event);
}

void
SpecvizPrivate::enable(bool enable)
{}

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
        qDebug() << ss->compiled();
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
    d.ui->plotWidget->replot();
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
    d.ui->plotWidget->xAxis->setLabel("Wavelength (nm)");
    d.ui->plotWidget->yAxis->setLabel("Relative Sensitivity");
    d.ui->plotWidget->rescaleAxes();
    d.ui->plotWidget->replot();
}

void
SpecvizPrivate::clear()
{}

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
        if (urls.size() == 1) {
            QString filename = urls.first().toLocalFile();
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
    if (urls.size() == 1) {
        QString filename = urls.first().toLocalFile();
        QString extension = QFileInfo(filename).suffix().toLower();
        if (p->d.extensions.contains(extension)) {
            if (p->loadDataset(filename)) {
                p->setSettingsValue("openDir", QFileInfo(filename).absolutePath());
            }
            else {
                qWarning() << "Could not load dataset from filename: " << filename;
            }
        }
    }
}
