/*********
*
* In the name of the Father, and of the Son, and of the Holy Spirit.
*
* This file is part of BibleTime's source code, http://www.bibletime.info/
*
* Copyright 1999-2020 by the BibleTime developers.
* The BibleTime source code is licensed under the GNU General Public License
* version 2.0.
*
**********/

import BibleTime 1.0
import QtQuick 2.2

Rectangle {
    id: display

    property int contextMenuIndex: btQmlInterface.contextMenuIndex

    // Mouse movement properties
    property int dragDistance: 8
    property int mouseMovedX: 0
    property int mouseMovedY: 0
    property int mousePressedX: 0
    property int mousePressedY: 0
    property point mouseLR
    property point mouseUL
    property bool draggingInProgress: false
    property bool selectionInProgress: false
    property string pressedLink: ""

    // Text selection properties
    property int column: 0          // Column containing selected text
    property int indexFirst: -1     // Index of first delegate item with selected text
    property int indexLast: -1      // Index of last delegate item with selected text
    property int textPosFirst: -1   // Position of first selected character
    property int textPosLast: -1    // Position of last selected character

    function saveContextMenuIndex(x, y) {
        contextMenuIndex = listView.indexAt(x,y+listView.contentY);
    }

    function getSelectedColumn() {
        return column;
    }

    function getFirstSelectedIndex() {
        return indexFirst;
    }

    function getLastSelectedIndex() {
        return indexLast;
    }

    function leftMousePress(x, y) {
        // Save cursor position for later use
        mousePressedX = x;
        mousePressedY = y;
        var delegateItem = listView.itemAt( x, y + listView.contentY);
        if (delegateItem === null)
            return;
        // Transform to delegate relative point
        var delegateX = x - delegateItem.x + listView.contentX;
        var delegateY = y - delegateItem.y + listView.contentY;
        pressedLink = delegateItem.linkAt(delegateX, delegateY);
    }

    function leftMouseMove(x, y) {
        draggingInProgress = false;
        selectionInProgress = false;
        mouseMovedX = x;
        mouseMovedY = y;
        // Do nothing unless mouse moved some distance from pressed location
        if ((Math.abs(mousePressedX - x) < dragDistance) && (Math.abs(mousePressedY - y) < dragDistance)) {
            return;
        }
        if (isCrossReference(pressedLink)) {
            // Start drag operation for cross reference link
            if (!draggingInProgress) {
                draggingInProgress = true;
                var index = listView.indexAt( mousePressedX, mousePressedY + listView.contentY);
                btQmlInterface.dragHandler(index, pressedLink);
            }
        } else {
            // Do text selection
            column = Math.floor(mousePressedX / (listView.width / listView.columns));
            sortMousePoints();
            getSelectedTextPositions();
            selectByIndex();
            selectionInProgress = true;
        }
    }

    function leftMouseRelease(x, y) {
        processMouseRelease(x, y);
        draggingInProgress = false;
        selectionInProgress = false;
    }

    function processMouseRelease(x, y) {
        if (draggingInProgress)
            return;
        if (selectionInProgress)
            return;
        deselectByIndex();
        if (openPersonalCommentary(mousePressedX, mousePressedY))
            return;
        if (! isCrossReference(pressedLink))
            return;
        btQmlInterface.setKeyFromLink(pressedLink);
    }

    function isCrossReference(url) {
        if (url === "" || url.includes("lemmamorph") || url.includes("footnote"))
            return false;
        return true;
    }

    function sortMousePoints() {
        if (mousePressedX < mouseMovedX) {
            if (mousePressedY < mouseMovedY) {
                mouseUL = Qt.point(mousePressedX, mousePressedY);
                mouseLR = Qt.point(mouseMovedX, mouseMovedY);
            } else {
                mouseUL = Qt.point(mousePressedX, mouseMovedY);
                mouseLR = Qt.point(mouseMovedX, mousePressedY);
            }
        } else {
            if (mousePressedY < mouseMovedY) {
                mouseUL = Qt.point(mouseMovedX, mousePressedY);
                mouseLR = Qt.point(mousePressedX, mouseMovedY);
            } else {
                mouseUL = Qt.point(mouseMovedX, mouseMovedY);
                mouseLR = Qt.point(mousePressedX, mousePressedY);
            }
        }
    }

    function getSelectedTextPositions() {
        var firstDelegateItem = listView.itemAt(mouseUL.x, mouseUL.y + listView.contentY);
        var lastDelegateItem = listView.itemAt(mouseLR.x, mouseLR.y + listView.contentY);
        if (firstDelegateItem === null || lastDelegateItem === null)
            return;

        indexFirst = listView.indexAt(mouseUL.x, mouseUL.y + listView.contentY);
        var delegateXFirst = listView.contentX + mouseUL.x - firstDelegateItem.x;
        var delegateYFirst = listView.contentY + mouseUL.y - firstDelegateItem.y;
        textPosFirst = firstDelegateItem.positionAt(delegateXFirst, delegateYFirst, column);

        indexLast = listView.indexAt(mouseLR.x, mouseLR.y + listView.contentY);
        var delegateXLast = listView.contentX + mouseLR.x - lastDelegateItem.x;
        var delegateYLast = listView.contentY + mouseLR.y - lastDelegateItem.y;
        textPosLast = lastDelegateItem.positionAt(delegateXLast, delegateYLast, column);
    }

    function selectDelegateItem(index, delegateItem) {
        if (delegateItem === null)
            return;
        if (index === indexFirst && index === indexLast)
            delegateItem.selectSingle(column, textPosFirst, textPosLast);
        else if (index === indexFirst)
            delegateItem.selectFirst(column, textPosFirst);
        else if (index === indexLast)
            delegateItem.selectLast(column, textPosLast);
        else if (index > indexFirst && index < indexLast)
            delegateItem.selectAll(column);
        else
            return;

        var columnSelectedText = delegateItem.getSelectedText(column);
        btQmlInterface.saveSelectedText(index, columnSelectedText)
    }

    function selectByIndex() {
        if (indexFirst < 0 || indexLast < 0 || indexFirst > indexLast)
            return;

        var index;
        for (index = indexFirst; index <= indexLast; index++) {
            var delegateItem = listView.itemAtIndex(index);
            selectDelegateItem(index, delegateItem);
        }
    }

    function deselectByIndex() {
        if (indexFirst < 0 || indexLast < 0 || indexFirst > indexLast)
            return;
        var index;
        for (index = indexFirst; index <= indexLast; index++) {
            var delegateItem = listView.itemAtIndex(index);
            if (delegateItem !== null)
                delegateItem.deselect(column);
        }
    }

    function openPersonalCommentary(x, y) {
        var index = listView.indexAt( x,y + listView.contentY);
        var column = Math.floor(x / (listView.width / listView.columns));
        return listView.startEdit(index, column);
    }

    function updateReferenceText() {
        listView.updateReferenceText();
    }

    function scroll(value) {
        listView.scroll(value);
    }

    function textIsHtml(dtext) {
        if (dtext.includes("<!DOCTYPE") || dtext.includes("<span"))
            return Text.RichText;
        return Text.PlainText;
    }

    function isBlank(text) {
        var t = text;
        t.trim();
        return t.length === 0;
    }

    width: 10
    height: 10
    color: btQmlInterface.backgroundColor

    BtQmlInterface {
        id: btQmlInterface

        onPageDownChanged: {
            listView.scroll(listView.height * 0.8);
            updateReferenceText();
        }
        onPageUpChanged: {
            listView.scroll(listView.height * -0.8);
            updateReferenceText();
        }
        onPositionItemOnScreen: {
            listView.positionViewAtIndex(index, ListView.Contain);
            updateReferenceText();
        }
    }

    ListView {
        id: listView

        property color textColor: btQmlInterface.foregroundColor
        property color textBackgroundColor: btQmlInterface.backgroundColor
        property int columns: btQmlInterface.numModules
        property int savedRow: 0
        property int savedColumn: 0
        property int backgroundHighlightIndex: btQmlInterface.backgroundHighlightColorIndex

        function scroll(value) {
            var y = contentY;
            contentY = y + value;
        }

        function startEdit(row, column) {
            if (!btQmlInterface.moduleIsWritable(column))
                return false;
            savedRow = row;
            savedColumn = column;
            btQmlInterface.openEditor(row, column);
            return true;
        }

        function finishEdit(newText) {
            btQmlInterface.setRawText(savedRow, savedColumn, newText)
        }

        function updateReferenceText() {
            var index = indexAt(contentX,contentY+30);
            btQmlInterface.changeReference(index);
        }

        clip: true
        anchors.fill: parent
        anchors.leftMargin: 0
        anchors.rightMargin: 0
        anchors.topMargin: 0
        anchors.bottomMargin: 0
        focus: true
        maximumFlickVelocity: 900
        model: btQmlInterface.textModel
        spacing: 2
        highlightFollowsCurrentItem: true
        currentIndex: btQmlInterface.currentModelIndex
        onCurrentIndexChanged: {
            positionViewAtIndex(currentIndex,ListView.Beginning)
        }
        onMovementEnded: {
            updateReferenceText();
        }

        delegate: DisplayDelegate {
            id: dd
            Component.onCompleted: {
                selectDelegateItem(index, dd);
            }
        }
    }
}
