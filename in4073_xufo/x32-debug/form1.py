# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'form1.ui'
#
# Created: Fri Jun 9 02:33:41 2006
#      by: The PyQt User Interface Compiler (pyuic) 3.14.1
#
# WARNING! All changes made in this file will be lost!


from qt import *
from qtext import *

class terminal(QTextEdit):
    def keyPressEvent(self, e):
        if keypress(e):
            QTextEdit.keyPressEvent(self, e)
        self.last_x = self.getCursorPosition()[1]

    def contentsMouseReleaseEvent(self, e):
        if e.button() == QMouseEvent.LeftButton:
            QTextEdit.contentsMouseReleaseEvent(self, e)
        elif e.button() == QMouseEvent.MidButton:
            y, x = self.charAt(e.pos()) 
            if y == self.paragraphs()-1:
                QTextEdit.contentsMouseReleaseEvent(self, e)

    def contentsDropEvent(self, e): pass
    def createPopupMenu(self, *args): pass

class Form1(QMainWindow):
    def __init__(self,parent = None,name = None,fl = 0, fontsize=10):
        QMainWindow.__init__(self,parent,name,fl)
        #self.statusBar()

        if not name:
            self.setName("Form1")

        self.setCentralWidget(QWidget(self,"qt_central_widget"))
        Form1Layout = QHBoxLayout(self.centralWidget(),11,8,"Form1Layout")

        layout3 = QVBoxLayout(None,0,8,"layout3")

        #self.textEdit1 = QTextEdit(self.centralWidget(),"textEdit1")
        self.textEdit1 = QextScintilla(self.centralWidget())
        lexer = QextScintillaLexerCPP(self.textEdit1)

        self.textEdit1.setLexer(lexer)

        self.textEdit1.setMarginWidth(0,32)
        self.textEdit1.setMarginWidth(1,16)

        self.textEdit1.SendScintilla(self.textEdit1.SCI_SETHSCROLLBAR, 0, 0)
        for i in range(100):
            self.textEdit1.SendScintilla(self.textEdit1.SCI_STYLESETFONT, i, "Monospace")
            self.textEdit1.SendScintilla(self.textEdit1.SCI_STYLESETSIZE, i, fontsize)
            self.textEdit1.SendScintilla(self.textEdit1.SCI_STYLESETBOLD, 0)

        self.textEdit1.setSizePolicy(QSizePolicy(QSizePolicy.Expanding,QSizePolicy.Expanding,6,7,self.textEdit1.sizePolicy().hasHeightForWidth()))
        #textEdit1_font = QFont(self.textEdit1.font())
        #textEdit1_font.setFamily("Monospace")
        #textEdit1_font.setPointSize(t)
        #self.textEdit1.setFont(textEdit1_font)

        layout3.addWidget(self.textEdit1)

        self.textEdit2 = terminal(self.centralWidget(),"textEdit2")
        #self.textEdit2.setReadOnly(1)
        self.textEdit2.setSizePolicy(QSizePolicy(QSizePolicy.Expanding,QSizePolicy.Expanding,6,3,self.textEdit2.sizePolicy().hasHeightForWidth()))
        textEdit2_font = QFont(self.textEdit2.font())
        textEdit2_font.setFamily("Monospace")
        textEdit2_font.setPointSize(fontsize)
        self.textEdit2.setFont(textEdit2_font)
        self.textEdit2.setWordWrap(QTextEdit.WidgetWidth)
        layout3.addWidget(self.textEdit2)
        Form1Layout.addLayout(layout3)

        layout14 = QVBoxLayout(None,0,16,"layout14")

        layout12 = QHBoxLayout(None,0,6,"layout12")
        self.textLabel1 = QLabel(self.centralWidget(),"textLabel1")
        layout12.addWidget(self.textLabel1)

        self.lineEdit1 = QLineEdit(self.centralWidget(),"lineEdit1")
        self.lineEdit1.setSizePolicy(QSizePolicy(QSizePolicy.Fixed,QSizePolicy.Fixed,2,0,self.lineEdit1.sizePolicy().hasHeightForWidth()))
        layout12.addWidget(self.lineEdit1)
        layout14.addLayout(layout12)

        layout13 = QHBoxLayout(None,0,6,"layout13")
        self.textLabel1_2 = QLabel(self.centralWidget(),"textLabel1_2")
        layout13.addWidget(self.textLabel1_2)

        self.lineEdit1_2 = QLineEdit(self.centralWidget(),"lineEdit1_2")
        self.lineEdit1_2.setSizePolicy(QSizePolicy(QSizePolicy.Fixed,QSizePolicy.Fixed,1,0,self.lineEdit1_2.sizePolicy().hasHeightForWidth()))
        layout13.addWidget(self.lineEdit1_2)
        layout14.addLayout(layout13)

        layout15 = QHBoxLayout(None,0,6,"layout15")
        self.textLabel1_3 = QLabel(self.centralWidget(),"textLabel1_3")
        layout15.addWidget(self.textLabel1_3)

        self.lineEdit1_3 = QLineEdit(self.centralWidget(),"lineEdit1_3")
        self.lineEdit1_3.setSizePolicy(QSizePolicy(QSizePolicy.Fixed,QSizePolicy.Fixed,1,0,self.lineEdit1_2.sizePolicy().hasHeightForWidth()))
        layout15.addWidget(self.lineEdit1_3)
        layout14.addLayout(layout15)


        spacer3 = QSpacerItem(20,391,QSizePolicy.Minimum,QSizePolicy.Expanding)
        layout14.addItem(spacer3)

        Form1Layout.addLayout(layout14)

        self.languageChange()

        self.resize(QSize(850,663).expandedTo(self.minimumSizeHint()))
        self.clearWState(Qt.WState_Polished)


    def languageChange(self):
        self.setCaption(self.__tr("X32 Debugger"))
        self.textEdit2.setText(self.__tr("blahblah"))
        self.textLabel1.setText(self.__tr("PC:"))
        self.lineEdit1.setText(QString.null)
        self.textLabel1_2.setText(self.__tr("SP:"))
        self.lineEdit1_2.setText(QString.null)
        self.textLabel1_3.setText(self.__tr("EL:"))
        self.lineEdit1_3.setText(QString.null)


    def __tr(self,s,c = None):
        return qApp.translate("Form1",s,c)
