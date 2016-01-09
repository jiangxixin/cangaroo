#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtWidgets>
#include <QMdiArea>
#include <QSignalMapper>
#include <QCloseEvent>
#include <QDebug>
#include <QDomDocument>

#include "Logger.h"
#include <model/MeasurementSetup.h>
#include <window/TraceWindow/TraceWindow.h>
#include <window/SetupDialog/SetupDialog.h>
#include <window/LogWindow/LogWindow.h>
#include <window/GraphWindow/GraphWindow.h>

MainWindow::MainWindow(Logger *logger, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _logger(logger)
{
    ui->setupUi(this);

    QIcon icon(":/assets/icon.png");
    setWindowIcon(icon);

    QImage bgimg(":/assets/mdibg.png");
    ui->mdiArea->setBackground(bgimg);
    ui->mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    connect(ui->action_Trace_View, SIGNAL(triggered()), this, SLOT(createTraceWindow()));
    connect(ui->actionLog_View, SIGNAL(triggered()), this, SLOT(createLogWindow()));
    connect(ui->actionGraph_View, SIGNAL(triggered()), this, SLOT(createGraphWindow()));
    connect(ui->actionSetup, SIGNAL(triggered()), this, SLOT(showSetupDialog()));

    connect(ui->actionStart_Measurement, SIGNAL(triggered()), this, SLOT(startMeasurement()));
    connect(ui->actionStop_Measurement, SIGNAL(triggered()), this, SLOT(stopMeasurement()));

    connect(&backend, SIGNAL(beginMeasurement()), this, SLOT(updateMeasurementActions()));
    connect(&backend, SIGNAL(endMeasurement()), this, SLOT(updateMeasurementActions()));
    updateMeasurementActions();

    connect(ui->actionSave_Trace_to_file, SIGNAL(triggered(bool)), this, SLOT(saveTraceToFile()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAboutDialog()));

    windowMapper = new QSignalMapper(this);
    connect(windowMapper, SIGNAL(mapped(QWidget*)), this, SLOT(setActiveSubWindow(QWidget*)));

    backend.addCanDriver(&_socketcan);
    backend.setSetup(backend.createDefaultSetup());

    QMdiSubWindow *logWindow = createLogWindow();
    logWindow->setGeometry(0, 500, 1000, 200);

    QMdiSubWindow *traceViewWindow = createTraceWindow();
    traceViewWindow->setGeometry(0, 0, 1000, 500);

    /*QMdiSubWindow *graphViewWindow = createGraphView();
    graphViewWindow->setGeometry(0, 500, 1000, 200);
*/
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateMeasurementActions()
{
    bool running = backend.isMeasurementRunning();
    ui->actionStart_Measurement->setEnabled(!running);
    ui->actionStop_Measurement->setEnabled(running);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    ui->mdiArea->closeAllSubWindows();
    if (ui->mdiArea->currentSubWindow()) {
        event->ignore();
    } else {
        //writeSettings();
        event->accept();
    }
}

QMdiSubWindow *MainWindow::createSubWindow(QWidget *window)
{
    QMdiSubWindow *retval = ui->mdiArea->addSubWindow(window);
    window->show();
    return retval;
}

void MainWindow::loadWorkspace(QString filename)
{
    _workspaceFileName = filename;
    // TODO implement me
}

void MainWindow::saveWorkspace(QString filename)
{
    QDomDocument doc;
    QDomElement root = doc.createElement("cangaroo-workspace");
    doc.appendChild(root);

    QDomElement windowsRoot = doc.createElement("windows");
    root.appendChild(windowsRoot);

    foreach (QMdiSubWindow *window, ui->mdiArea->subWindowList()) {
        QDomElement wnode = doc.createElement("window");
        wnode.setAttribute("left", window->geometry().left());
        wnode.setAttribute("top", window->geometry().top());
        wnode.setAttribute("width", window->geometry().width());
        wnode.setAttribute("height", window->geometry().height());

        MdiWindow *mdiwin = dynamic_cast<MdiWindow*>(window->widget());
        if (!mdiwin->saveXML(backend, doc, wnode)) {
            qCritical() << "Cannot save window settings to file";
            return;
        }

        windowsRoot.appendChild(wnode);
    }

    QDomElement setupRoot = doc.createElement("setup");
    if (!backend.getSetup()->saveXML(backend, doc, setupRoot)) {
        qCritical() << "Cannot save measurement setup to file";
        return;
    }
    root.appendChild(setupRoot);

    QFile outFile(filename);
    if(outFile.open(QIODevice::WriteOnly|QIODevice::Text) ) {
        QTextStream stream( &outFile );
        stream << doc.toString();
        outFile.close();
        _workspaceFileName = filename;
    } else {
        qCritical() << "Cannot open workspace file for writing:" << filename;
    }
}

QMdiSubWindow *MainWindow::createTraceWindow() {
    return createSubWindow(new TraceWindow(ui->mdiArea, backend));
}

QMdiSubWindow *MainWindow::createLogWindow()
{
    return createSubWindow(new LogWindow(ui->mdiArea, *_logger));
}

QMdiSubWindow *MainWindow::createGraphWindow()
{
    return createSubWindow(new GraphWindow(ui->mdiArea, backend));
}

void MainWindow::setActiveSubWindow(QWidget *window) {
    if (window) {
        ui->mdiArea->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(window));
    }
}

bool MainWindow::showSetupDialog()
{
    SetupDialog dlg(backend, 0);
    MeasurementSetup *new_setup = dlg.showSetupDialog(*backend.getSetup());
    if (new_setup) {
        backend.setSetup(new_setup);
        return true;
    } else {
        return false;
    }
}

void MainWindow::showAboutDialog()
{
    QMessageBox::about(this, "About cangaroo", "cangaroo version 0.1\n(c)2015-2016 Hubert Denkmair");
}

void MainWindow::startMeasurement()
{
    if (showSetupDialog()) {
        backend.startMeasurement();
    }
}

void MainWindow::stopMeasurement()
{
    backend.stopMeasurement();
}

void MainWindow::saveTraceToFile()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save Trace to file", "", "Candump Files (*.candump)");
    if (!filename.isNull()) {
        backend.saveCanDump(filename);
    }
}


void MainWindow::on_action_TraceClear_triggered()
{
    backend.clearTrace();
}

void MainWindow::on_action_WorkspaceOpen_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open workspace configuration", "", "Workspace config files (*.cangaroo)");
    if (!filename.isNull()) {
        loadWorkspace(filename);
    }
}

void MainWindow::on_actionSave_as_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save workspace configuration", "", "Workspace config files (*.cangaroo)");
    if (!filename.isNull()) {
        saveWorkspace(filename);
    }
}
