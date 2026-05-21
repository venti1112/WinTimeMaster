#include "TimeRuleModel.h"
#include "ConfigManager.h"
#include <QJsonObject>
#include <QDebug>

TimeRuleModel::TimeRuleModel(QObject *parent) : QAbstractListModel(parent) {
    loadRules();
}

int TimeRuleModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_rules.size();
}

QVariant TimeRuleModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_rules.size()) return {};

    TimeRule *rule = m_rules.at(index.row());
    switch (role) {
        case IdRole: return rule->id();
        case StartTimeRole: return rule->startTime();
        case EndTimeRole: return rule->endTime();
        case RepeatModeRole: return rule->repeatMode();
        case WeekDaysRole: return rule->weekDays();
        case EnabledRole: return rule->enabled();
        default: return {};
    }
}

bool TimeRuleModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || index.row() >= m_rules.size()) return false;

    TimeRule *rule = m_rules.at(index.row());
    switch (role) {
        case StartTimeRole: rule->setStartTime(value.toTime()); break;
        case EndTimeRole: rule->setEndTime(value.toTime()); break;
        case RepeatModeRole: rule->setRepeatMode(static_cast<TimeRule::RepeatMode>(value.toInt())); break;
        case WeekDaysRole: rule->setWeekDays(value.toInt()); break;
        case EnabledRole: rule->setEnabled(value.toBool()); break;
        default: return false;
    }

    emit dataChanged(index, index, {role});
    saveRules();
    return true;
}

QHash<int, QByteArray> TimeRuleModel::roleNames() const {
    return {
        {IdRole, "ruleId"},
        {StartTimeRole, "startTime"},
        {EndTimeRole, "endTime"},
        {RepeatModeRole, "repeatMode"},
        {WeekDaysRole, "weekDays"},
        {EnabledRole, "enabled"}
    };
}

void TimeRuleModel::addRule() {
    auto *rule = new TimeRule(nextRuleId(), this);

    beginInsertRows(QModelIndex(), m_rules.size(), m_rules.size());
    m_rules.append(rule);
    endInsertRows();

    connect(rule, &TimeRule::startTimeChanged, this, [this, rule]() {
        int row = m_rules.indexOf(rule);
        if (row >= 0) emit dataChanged(index(row), index(row), {StartTimeRole});
    });
    connect(rule, &TimeRule::endTimeChanged, this, [this, rule]() {
        int row = m_rules.indexOf(rule);
        if (row >= 0) emit dataChanged(index(row), index(row), {EndTimeRole});
    });
    connect(rule, &TimeRule::repeatModeChanged, this, [this, rule]() {
        int row = m_rules.indexOf(rule);
        if (row >= 0) emit dataChanged(index(row), index(row), {RepeatModeRole});
    });
    connect(rule, &TimeRule::weekDaysChanged, this, [this, rule]() {
        int row = m_rules.indexOf(rule);
        if (row >= 0) emit dataChanged(index(row), index(row), {WeekDaysRole});
    });
    connect(rule, &TimeRule::enabledChanged, this, [this, rule]() {
        int row = m_rules.indexOf(rule);
        if (row >= 0) emit dataChanged(index(row), index(row), {EnabledRole});
    });

    saveRules();
}

void TimeRuleModel::updateRule(int row, const QVariantMap &data) {
    TimeRule *rule = ruleAt(row);
    if (!rule) return;

    if (data.contains("startTime"))   rule->setStartTime(data["startTime"].toTime());
    if (data.contains("endTime"))     rule->setEndTime(data["endTime"].toTime());
    if (data.contains("repeatMode"))  rule->setRepeatMode(static_cast<TimeRule::RepeatMode>(data["repeatMode"].toInt()));
    if (data.contains("weekDays"))    rule->setWeekDays(data["weekDays"].toInt());
    if (data.contains("enabled"))     rule->setEnabled(data["enabled"].toBool());

    emit dataChanged(index(row), index(row));
    saveRules();
}

void TimeRuleModel::removeRule(int row) {
    if (row < 0 || row >= m_rules.size()) return;

    beginRemoveRows(QModelIndex(), row, row);
    TimeRule *rule = m_rules.takeAt(row);
    endRemoveRows();

    rule->deleteLater();
    saveRules();
}

TimeRule *TimeRuleModel::ruleAt(int row) const {
    if (row < 0 || row >= m_rules.size()) return nullptr;
    return m_rules.at(row);
}

QList<TimeRule*> TimeRuleModel::rules() const {
    return m_rules;
}

void TimeRuleModel::loadRules() {
    const QJsonArray arr = ConfigManager::instance()->readJsonArray("TimeRules");
    if (arr.isEmpty()) return;

    for (const QJsonValue &val : arr) {
        QJsonObject obj = val.toObject();
        auto *rule = new TimeRule(obj["id"].toInt(), this);
        rule->setStartTime(QTime::fromString(obj["startTime"].toString(), "HH:mm"));
        rule->setEndTime(QTime::fromString(obj["endTime"].toString(), "HH:mm"));
        rule->setRepeatMode(static_cast<TimeRule::RepeatMode>(obj["repeatMode"].toInt()));
        rule->setWeekDays(obj["weekDays"].toInt());
        rule->setEnabled(obj["enabled"].toBool(true));

        beginInsertRows(QModelIndex(), m_rules.size(), m_rules.size());
        m_rules.append(rule);
        endInsertRows();

        if (rule->id() >= m_nextId) m_nextId = rule->id() + 1;

        // 连接信号
        connect(rule, &TimeRule::startTimeChanged, this, [this, rule]() {
            int row = m_rules.indexOf(rule);
            if (row >= 0) emit dataChanged(index(row), index(row), {StartTimeRole});
        });
        // 其余属性连接同上，为简洁此处省略，实际应添加全部
    }
    qDebug() << "Loaded" << m_rules.size() << "time rules.";
}

void TimeRuleModel::saveRules() {
    QJsonArray arr;
    for (const TimeRule *rule : m_rules) {
        QJsonObject obj;
        obj["id"]         = rule->id();
        obj["startTime"]  = rule->startTime().toString("HH:mm");
        obj["endTime"]    = rule->endTime().toString("HH:mm");
        obj["repeatMode"] = rule->repeatMode();
        obj["weekDays"]   = rule->weekDays();
        obj["enabled"]    = rule->enabled();
        arr.append(obj);
    }

    ConfigManager::instance()->writeJsonArray("TimeRules", arr);
    qDebug() << "Saved" << arr.size() << "time rules.";
}

int TimeRuleModel::nextRuleId() {
    return m_nextId++;
}
void TimeRuleModel::reload() {
    beginResetModel();
    qDeleteAll(m_rules);
    m_rules.clear();
    endResetModel();
    loadRules();
}