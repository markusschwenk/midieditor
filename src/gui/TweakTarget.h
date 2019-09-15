/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
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

#ifndef TWEAKTARGET_H_
#define TWEAKTARGET_H_

class MainWindow;

class TweakTarget {
public:
    virtual ~TweakTarget() {};
    virtual void smallDecrease() = 0;
    virtual void smallIncrease() = 0;
    virtual void mediumDecrease() = 0;
    virtual void mediumIncrease() = 0;
    virtual void largeDecrease() = 0;
    virtual void largeIncrease() = 0;
};

class TimeTweakTarget : public TweakTarget {
public:
    TimeTweakTarget(MainWindow* mainWindow);
    void smallDecrease();
    void smallIncrease();
    void mediumDecrease();
    void mediumIncrease();
    void largeDecrease();
    void largeIncrease();

protected:
    MainWindow* mainWindow;
    void offset(int amount);
};

class StartTimeTweakTarget : public TweakTarget {
public:
    StartTimeTweakTarget(MainWindow* mainWindow);
    void smallDecrease();
    void smallIncrease();
    void mediumDecrease();
    void mediumIncrease();
    void largeDecrease();
    void largeIncrease();

protected:
    MainWindow* mainWindow;
    void offset(int amount);
};

class EndTimeTweakTarget : public TweakTarget {
public:
    EndTimeTweakTarget(MainWindow* mainWindow);
    void smallDecrease();
    void smallIncrease();
    void mediumDecrease();
    void mediumIncrease();
    void largeDecrease();
    void largeIncrease();

protected:
    MainWindow* mainWindow;
    void offset(int amount);
};

class NoteTweakTarget : public TweakTarget {
public:
    NoteTweakTarget(MainWindow* mainWindow);
    void smallDecrease();
    void smallIncrease();
    void mediumDecrease();
    void mediumIncrease();
    void largeDecrease();
    void largeIncrease();

protected:
    MainWindow* mainWindow;
    void offset(int amount);
};

class ValueTweakTarget : public TweakTarget {
public:
    ValueTweakTarget(MainWindow* mainWindow);
    void smallDecrease();
    void smallIncrease();
    void mediumDecrease();
    void mediumIncrease();
    void largeDecrease();
    void largeIncrease();

protected:
    MainWindow* mainWindow;
    void offset(int amount, int pitchBendAmount, int tempoAmount);
};

#endif
