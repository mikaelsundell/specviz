// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#pragma once

#include <QDateTime>
#include <QObject>
#include <QUrl>

namespace Github {
struct Asset {
    QString name;
    QUrl url;
};

struct Release {
public:
    QString tag;
    QString title;
    QString notes;
    QUrl url;
    QDateTime published;
    QList<Asset> assets;
};
}  // namespace Github

class GithubClientPrivate;
class GithubClient : public QObject {
    Q_OBJECT
public:
    explicit GithubClient(QObject* parent = nullptr);
    virtual ~GithubClient();

    void setRepository(const QString& owner, const QString& repository);

Q_SIGNALS:
    void releasesReceived(const QList<Github::Release>& releases);
    void errorOccurred(const QString& error);

private:
    QScopedPointer<GithubClientPrivate> p;
};
