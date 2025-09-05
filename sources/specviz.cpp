// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#include "specviz.h"
#include "icctransform.h"
#include "platform.h"
#include "specfile.h"
#include "stylesheet.h"
#include "qcustomplot/qcustomplot.h"
#include "widgets.h"
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
        void itemChanged(QTreeWidgetItem *item, int column);
        void showSelected();
        void hideSelected();
        void deleteSelected();
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
    // tree
    tree()->setHeaderLabels(QStringList() << "Dataset/ channel" << "Source");
    tree()->setColumnWidth(TreeItem::Key, 160);
    tree()->setColumnWidth(TreeItem::Value, 100);
    // connect
    connect(d.ui->fileOpen, &QAction::triggered, this, &SpecvizPrivate::open);
    connect(d.ui->treeWidget, &QTreeWidget::itemChanged, this, &SpecvizPrivate::itemChanged);
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

bool
SpecvizPrivate::loadDataset(const QString& filename)
{
    SpecFile spec(filename);
    if (!spec.isLoaded()) {
        return false;
    }
    
    auto ds = spec.data();
    QTreeWidgetItem *rootItem = new QTreeWidgetItem(tree());
    rootItem->setText(0, ds.header.value("model").toString());
    rootItem->setText(1, QFileInfo(filename).fileName());
    rootItem->setCheckState(0, Qt::Checked);
    rootItem->setData(0, Qt::UserRole, QVariant::fromValue(d.datasets.size()));

    for (int i = 0; i < ds.indices.size(); ++i) {
        d.ui->plotWidget->addGraph();
        int graphIndex = d.ui->plotWidget->graphCount() - 1;
        QCPGraph *graph = d.ui->plotWidget->graph(graphIndex);
        graph->setName(ds.indices[i]);
        
        QString idx = ds.indices[i].toUpper();
        if (idx == "R") {
            graph->setPen(QPen(Qt::red, 2));
        }
        else if (idx == "G") {
            graph->setPen(QPen(Qt::green, 2));
        }
        else if (idx == "B") {
            graph->setPen(QPen(Qt::blue, 2));
        }
        else {
            graph->setPen(QPen(QColor::fromHslF((i * 0.15), 0.7, 0.5), 2));
        }

        QVector<double> x, y;
        x.reserve(ds.data.size());
        y.reserve(ds.data.size());

        for (auto it = ds.data.begin(); it != ds.data.end(); ++it) {
            x << it.key();               // wavelength
            if (i < it.value().size()) { // channel safety
                y << it.value().at(i);
            }
            else {
                y << 0.0;
            }
        }
        graph->setData(x, y);
        QTreeWidgetItem *child = new QTreeWidgetItem(rootItem);
        child->setText(0, ds.indices[i]);
        child->setCheckState(0, Qt::Checked);
        child->setData(0, Qt::UserRole, graphIndex);
        d.ui->plotWidget->graph(graphIndex)->setVisible(true);
    }

    tree()->expandItem(rootItem);
    d.datasets.append(ds);
    
    d.ui->plotWidget->legend->setVisible(true);
    d.ui->plotWidget->xAxis->setLabel("Wavelength (nm)");
    d.ui->plotWidget->yAxis->setLabel("Relative Sensitivity");
    d.ui->plotWidget->rescaleAxes();
    d.ui->plotWidget->replot();

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
{
    //d.ui->collapse->setEnabled(enable);
    //d.ui->expand->setEnabled(enable);
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
    QString path = platform::getApplicationPath() + "/Resources/App.css";
    auto ss = Stylesheet::instance();
    ss->setColor("basecolor", QColor::fromHslF(220.0 / 360.0, 0.3, 0.06));
    ss->setColor("accent", QColor::fromHslF(216.0 / 360.0, 0.82, 0.20));
    if (ss->loadCss(path)) {
        qApp->setStyleSheet(ss->compiled());
    }
    // qcustomplot
    d.ui->plotWidget->setBackground(QBrush(QColor::fromHslF(220 / 360.0, 0.3, 0.06)));
    d.ui->plotWidget->axisRect()->setBackground(QBrush(QColor::fromHslF(220 / 360.0, 0.3, 0.06)));

    QPen axisPen(Qt::white);
    d.ui->plotWidget->xAxis->setBasePen(axisPen);
    d.ui->plotWidget->yAxis->setBasePen(axisPen);
    d.ui->plotWidget->xAxis->setTickPen(axisPen);
    d.ui->plotWidget->yAxis->setTickPen(axisPen);
    d.ui->plotWidget->xAxis->setSubTickPen(axisPen);
    d.ui->plotWidget->yAxis->setSubTickPen(axisPen);
    d.ui->plotWidget->xAxis->setTickLabelColor(Qt::white);
    d.ui->plotWidget->yAxis->setTickLabelColor(Qt::white);

    QPen gridPen(QColor(80, 80, 80));
    d.ui->plotWidget->xAxis->grid()->setPen(gridPen);
    d.ui->plotWidget->yAxis->grid()->setPen(gridPen);
    
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
SpecvizPrivate::itemChanged(QTreeWidgetItem *item, int column)
{
    if (!item->parent()) {
        Qt::CheckState rootState = item->checkState(0);
        for (int i = 0; i < item->childCount(); ++i) {
            QTreeWidgetItem *child = item->child(i);
            child->setCheckState(0, rootState);
        }
    } else {
        bool visible = (item->checkState(0) == Qt::Checked);
        int graphIndex = item->data(0, Qt::UserRole).toInt();
        if (graphIndex >= 0 && graphIndex < d.ui->plotWidget->graphCount()) {
            d.ui->plotWidget->graph(graphIndex)->setVisible(visible);
        }
        QTreeWidgetItem *parent = item->parent();
        int checkedCount = 0;
        for (int i = 0; i < parent->childCount(); ++i) {
            if (parent->child(i)->checkState(0) == Qt::Checked) {
                checkedCount++;
            }
        }
        if (checkedCount == parent->childCount()) {
            parent->setCheckState(0, Qt::Checked);
        }
        else if (checkedCount == 0) {
            parent->setCheckState(0, Qt::Unchecked);
        }
        else {
            parent->setCheckState(0, Qt::PartiallyChecked);
        }
    }
    d.ui->plotWidget->replot();
}

void
SpecvizPrivate::showSelected()
{
    //if (selection()->paths().size()) {
   //     d.controller->visiblePaths(d.selection->paths(), true);
   // }
}

void
SpecvizPrivate::hideSelected()
{
    //if (selection()->paths().size()) {
    //    d.controller->visiblePaths(d.selection->paths(), false);
    //}
}

void
SpecvizPrivate::deleteSelected()
{
    //if (selection()->paths().size()) {
    //    d.controller->removePaths(d.selection->paths());
    //}
}

void
SpecvizPrivate::clear()
{
    //d.ui->clear->setText(QString());
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
