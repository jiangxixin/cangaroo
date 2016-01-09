#include "TraceWindow.h"
#include "ui_TraceWindow.h"

#include <QDomDocument>
#include <QSortFilterProxyModel>
#include "LinearTraceViewModel.h"
#include "AggregatedTraceViewModel.h"

TraceWindow::TraceWindow(QWidget *parent, Backend &backend) :
    MdiWindow(parent),
    ui(new Ui::TraceWindow),
    _backend(&backend)
{
    ui->setupUi(this);

    _linearTraceViewModel = new LinearTraceViewModel(backend);
    _linearProxyModel = new QSortFilterProxyModel(this);
    _linearProxyModel->setSourceModel(_linearTraceViewModel);
    _linearProxyModel->setDynamicSortFilter(true);

    _aggregatedTraceViewModel = new AggregatedTraceViewModel(backend);
    _aggregatedProxyModel = new QSortFilterProxyModel(this);
    _aggregatedProxyModel->setSourceModel(_aggregatedTraceViewModel);
    _aggregatedProxyModel->setDynamicSortFilter(true);

    setMode(mode_linear);
    ui->tree->setUniformRowHeights(true);
    ui->tree->setColumnWidth(0, 80);
    ui->tree->setColumnWidth(1, 70);
    ui->tree->setColumnWidth(2, 50);
    ui->tree->setColumnWidth(3, 90);
    ui->tree->setColumnWidth(4, 200);
    ui->tree->setColumnWidth(5, 50);
    ui->tree->setColumnWidth(6, 200);


    connect(_linearTraceViewModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(rowsInserted(QModelIndex,int,int)));
    connect(ui->cbAggregated, SIGNAL(stateChanged(int)), this, SLOT(onCbTraceTypeChanged(int)));

}

TraceWindow::~TraceWindow()
{
    delete ui;
    delete _aggregatedTraceViewModel;
    delete _linearTraceViewModel;
}

void TraceWindow::setMode(TraceWindow::mode_t mode)
{
    _mode = mode;

    if (_mode==mode_linear) {
        ui->tree->setModel(_linearTraceViewModel);
        ui->tree->setSortingEnabled(false);
    } else {
        ui->tree->setModel(_aggregatedProxyModel);
        ui->tree->setSortingEnabled(true);
    }

}

bool TraceWindow::saveXML(Backend &backend, QDomDocument &xml, QDomElement &root)
{
    if (!MdiWindow::saveXML(backend, xml, root)) {
        return false;
    }

    root.setAttribute("type", "TraceWindow");
    root.setAttribute("mode", (_mode==mode_linear) ? "linear" : "aggregated");

    QDomElement elLinear = xml.createElement("LinearTraceView");
    elLinear.setAttribute("autoscroll", (ui->cbAutoScroll->checkState() == Qt::Checked) ? 1 : 0);
    root.appendChild(elLinear);

    QDomElement elAggregated = xml.createElement("AggregatedTraceView");
    elAggregated.setAttribute("SortColumn", _aggregatedProxyModel->sortColumn());
    root.appendChild(elAggregated);

    return true;
}

void TraceWindow::onCbTraceTypeChanged(int i)
{
    setMode( (i==Qt::Checked) ? mode_aggregated : mode_linear );
}

void TraceWindow::rowsInserted(const QModelIndex &parent, int first, int last)
{
    (void) parent;
    (void) first;
    (void) last;

    if ((_mode==mode_linear) && (ui->cbAutoScroll->checkState() == Qt::Checked)) {
        ui->tree->scrollToBottom();
    }

}
