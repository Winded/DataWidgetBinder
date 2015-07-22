#include "datawidgetbinder.h"
#include <QVariant>
#include <QWidget>
#include <QMetaProperty>
#include <assert.h>

DataWidgetBinder::DataWidgetBinder(QObject *parent) :
	QObject(parent),
	mModel(0),
	mEditingWidget(false),
	mEditingModel(false)
{
}

QAbstractItemModel *DataWidgetBinder::model() const {
	return mModel;
}

void DataWidgetBinder::setModel(QAbstractItemModel *model) {
	if(mModel) {
		disconnect(mModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(dataChanged(QModelIndex,QModelIndex)));
	}
	mModel = model;
	connect(mModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(dataChanged(QModelIndex,QModelIndex)));
	connect(mModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(rowsRemoved(QModelIndex,int,int)));
}

QModelIndex DataWidgetBinder::currentIndex() const {
	return mCurrentIndex;
}

void DataWidgetBinder::setCurrentIndex(QModelIndex index) {
	mCurrentIndex = index;
	updateValues();
}

void DataWidgetBinder::addBinding(QWidget *widget, QString property, int column, QString notifySignal) {
	assert(mModel);

	assert(widget->metaObject()->indexOfProperty(property.toStdString().c_str()) != -1);
	assert(!mBindings.contains(widget));

	if(notifySignal.isNull()) {
		QMetaProperty mp = widget->metaObject()->property(widget->metaObject()->indexOfProperty(property.toStdString().c_str()));
		notifySignal = signature(mp.notifySignal());
	}
	int sigId = widget->metaObject()->indexOfSignal(widget->metaObject()->normalizedSignature(notifySignal.toStdString().c_str()));
	int methodId = metaObject()->indexOfMethod(metaObject()->normalizedSignature("propertyChanged()"));
	metaObject()->connect(widget, sigId, this, methodId);

	Binding b;
	b.mWidget = widget;
	b.mProperty = property;
	b.mColumn = column;
	b.mNotifySignal = notifySignal;
	mBindings[widget] = b;
}

void DataWidgetBinder::removeBinding(QWidget *widget) {
	assert(mModel);
	assert(mBindings.contains(widget));

	Binding b = mBindings[widget];
	assert(widget->metaObject()->indexOfProperty(b.mProperty.toStdString().c_str()) != -1);
	int sigId = widget->metaObject()->indexOfSignal(widget->metaObject()->normalizedSignature(b.mNotifySignal.toStdString().c_str()));
	int methodId = metaObject()->indexOfMethod(metaObject()->normalizedSignature("propertyChanged()"));
	metaObject()->disconnect(widget, sigId, this, methodId);

	mBindings.remove(widget);
}

void DataWidgetBinder::propertyChanged() {
	if(mEditingWidget) {
		return;
	}
	if(!mModel || !mCurrentIndex.isValid()) {
		return;
	}
	mEditingModel = true;

	QWidget *widget = (QWidget*)sender();
	assert(mBindings.contains(widget));

	Binding b = mBindings[widget];
	QVariant value = widget->property(b.mProperty.toStdString().c_str());
	QModelIndex idx = mModel->index(mCurrentIndex.row(), b.mColumn, mCurrentIndex.parent());
	mModel->setData(idx, value, Qt::EditRole);

	mEditingModel = false;
}

void DataWidgetBinder::dataChanged(QModelIndex topLeft, QModelIndex bottomRight) {
	if(mEditingModel) {
		return;
	}
	if(mCurrentIndex.isValid() && mCurrentIndex.parent() == topLeft.parent() &&
			mCurrentIndex.row() >= topLeft.row() && mCurrentIndex.row() <= bottomRight.row()) {
		updateValues();
	}
}

void DataWidgetBinder::rowsRemoved(QModelIndex parent, int start, int end) {
	if(mEditingModel) {
		return;
	}
	if(mCurrentIndex.parent() == parent &&
			mCurrentIndex.row() >= start && mCurrentIndex.row() <= end) {
		setCurrentIndex(QModelIndex());
	}
}

void DataWidgetBinder::updateValues() {
	if(!mModel || !mCurrentIndex.isValid()) {
		return;
	}

	mEditingWidget = true;

	for(Binding b : mBindings) {
		QModelIndex idx = mModel->index(mCurrentIndex.row(), b.mColumn, mCurrentIndex.parent());
		QVariant value = mModel->data(idx, Qt::EditRole);
		b.mWidget->setProperty(b.mProperty.toStdString().c_str(), value);
	}

	mEditingWidget = false;
}

QString DataWidgetBinder::signature(QMetaMethod metaMethod) {
	QString str = metaMethod.name();
	str += "(";
	for(QByteArray t : metaMethod.parameterTypes()) {
		str += QString(t);
		str += "/";
	}
	str.chop(1);
	str += ")";
	return str;
}
