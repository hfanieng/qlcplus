include(../variables.pri)

TEMPLATE = subdirs

docs.path = $$INSTALLROOT/$$DOCSDIR/html
docs.files = \
             addeditfixtures.html \
             addresstool.html \
             addvcbuttonmatrix.html \
             artnetplugin.html \
             audiotriggers.html \
             capabilityeditor.html \
             capabilitywizard.html \
             channeleditor.html \
             channelsgroupeditor.html \
             chasereditor.html \
             collectioneditor.html \
             commandlineparameters.html \
             concept.html \
             dmxdump.html \
             dmxusbplugin.html \
             efxeditor.html \
             efx-general.png \
             efx-movement.png \
             fixturedefinitioneditor.html \
             fixtureeditor.html \
             fixturegroupeditor.html \
             fixturemanager.html \
             fixturemonitor.html \
             fixturesremap.html \
             fixremap.png \
             functionmanager.html \
             functionwizard.html \
             guicustomstyles.html \
             headeditor.html \
             howto-add-fixtures.html \
             howto-input-profiles.html \
             howto-input-output-mapping.html \
             index.html \
             liveedit.html \
             mainwindow.html \
             mainwindow2.png \
             midiplugin.html \
             modeeditor.html \
             olaplugin.html \
             oscplugin.html \
             parameterstuning.html \
             peperonioutput.html \
             questionsandanswers.html \
             rgbmatrixeditor.html \
             rgbscriptapi.html \
             sceneeditor.html \
             selectfunction.html \
             selectfixture.html \
             selectinputchannel.html \
             showeditor.html \
             showmanager.html \
             simpledesk.html \
             tutorial.html \
             tutorial1_1.png \
             tutorial1_2.png \
             tutorial1_3.png \
             udmxoutput.html \
             vcbutton.html \
             vcbuttonmatrix.html \
             vccuelist.html \
             vcframe.html \
             vclabel.html \
             vcsoloframe.html \
             vcspeeddial.html \
             vcslider.html \
             vcslidermatrix.html \
             vcstylingplacement.html \
             vcxypad.html \
             vellemanoutput.html \
             virtualconsole.html

INSTALLS += docs
