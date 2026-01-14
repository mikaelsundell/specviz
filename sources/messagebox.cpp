// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#include "messagebox.h"

#include <QPointer>

// generated files
#include "ui_messagebox.h"

class MessageBoxPrivate : public QObject {
    Q_OBJECT
public:
    MessageBoxPrivate();
    void init();
    bool exec();

public:
    struct Data {
        QString title;
        QString url;
        QString heading;
        QString details;
        QString acceptText;
        QString rejectText;
        bool showReject = false;
        QPointer<MessageBox> dialog;
        QScopedPointer<Ui_MessageBox> ui;
    };
    Data d;
};

MessageBoxPrivate::MessageBoxPrivate() {}

void
MessageBoxPrivate::init()
{
    // ui
    d.ui.reset(new Ui_MessageBox());
    d.ui->setupUi(d.dialog.data());
    // connect
    connect(d.ui->accept, &QPushButton::clicked, this, [this]() { d.dialog->done(QDialog::Accepted); });
    connect(d.ui->reject, &QPushButton::clicked, this, [this]() { d.dialog->done(QDialog::Rejected); });
}

bool
MessageBoxPrivate::exec()
{
    d.dialog->setWindowTitle(d.title);
    d.ui->title->setText(d.title);

    if (d.url.isEmpty()) {
        d.ui->url->hide();
    }
    else {
        d.ui->url->setText(QString("<a href='%1'>%1</a>").arg(d.url));
        d.ui->url->setOpenExternalLinks(true);
        d.ui->url->show();
    }

    if (d.heading.isEmpty()) {
        d.ui->heading->hide();
    }
    else {
        d.ui->heading->setText(d.heading);
        d.ui->heading->show();
    }

    if (d.details.isEmpty()) {
        d.ui->details->hide();
    }
    else {
        d.ui->details->setText(d.details);
        d.ui->details->show();
    }

    d.ui->accept->setText(d.acceptText);
    if (d.showReject) {
        d.ui->reject->setText(d.rejectText);
        d.ui->reject->show();
    }
    else {
        d.ui->reject->hide();
    }
    return d.dialog->exec() == QDialog::Accepted;
}

#include "messagebox.moc"

MessageBox::MessageBox(QWidget* parent)
    : QDialog(parent)
    , p(new MessageBoxPrivate())
{
    p->d.dialog = this;
    p->init();
}

MessageBox::~MessageBox() {}

bool
MessageBox::information(QWidget* parent, const QString& title, const QString& text)
{}

bool
MessageBox::warning(QWidget* parent, const QString& title, const QString& text)
{}

bool
MessageBox::question(QWidget* parent, const QString& title, const QString& text)
{}

bool
MessageBox::about(QWidget* parent, const QString& title, const QString& heading, const QString& details,
                  const QString& url)
{
    MessageBox box(parent);
    box.p->d.title = title;
    box.p->d.url = url;
    box.p->d.heading = heading;
    box.p->d.details = details;
    box.p->d.acceptText = tr("Close");
    box.p->d.showReject = false;
    return box.p->exec();
}

bool
MessageBox::update(QWidget* parent, const QString& title, const QString& heading, const QString& details,
                   const QString& url)
{
    MessageBox box(parent);
    box.p->d.title = title;
    box.p->d.url = url;
    box.p->d.heading = heading;
    box.p->d.details = details;
    box.p->d.acceptText = tr("Download");
    box.p->d.rejectText = tr("Skip");
    box.p->d.showReject = true;
    return box.p->exec();
}
