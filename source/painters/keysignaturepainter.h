/*
  * Copyright (C) 2011 Cameron White
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
  
#ifndef PAINTERS_KEYSIGNATUREPAINTER_H
#define PAINTERS_KEYSIGNATUREPAINTER_H

#include <QFont>
#include <QGraphicsItem>
#include <painters/layoutinfo.h>
#include <score/scorelocation.h>

class ScoreLocationPubSub;

class KeySignaturePainter : public QGraphicsItem
{
public:
    KeySignaturePainter(const LayoutConstPtr &layout,
                        const KeySignature &key,
                        const ScoreLocation &location,
                        boost::shared_ptr<ScoreLocationPubSub> pubsub);

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *,
               QWidget *);
    virtual QRectF boundingRect() const { return myBounds; }

private:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *);

    LayoutConstPtr myLayout;
    const KeySignature &myKeySignature;
    const ScoreLocation myLocation;
    boost::shared_ptr<ScoreLocationPubSub> myPubSub;
    QFont myMusicFont;
    const QRectF myBounds;
    QVector<double> myFlatPositions;
    QVector<double> mySharpPositions;

    void adjustHeightOffset(QVector<double> &lst);
    void drawAccidentals(QVector<double> &positions, QChar accidental,
                         QPainter *painter);
    void initAccidentalPositions();
};

#endif
