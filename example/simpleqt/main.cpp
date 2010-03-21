/* vim: ts=4 sw=4 sts=4 et
 *
 * Copyright (c) 2009 James R. McKaskill
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * ----------------------------------------------------------------------------
 */

#include "Main.hxx"
#include <QCoreApplication>
#include <qdbusmessage.h>
#include <qdebug.h>
#include <adbus.h>

Main::Main(const QDBusConnection& c)
{ 
    QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.DBus", "/", "org.freedesktop.DBus", "RequestName");
    msg << "nz.co.foobar.SimpleQtTest";
    msg << uint(0);
    c.callWithCallback(msg, this, SLOT(NameRequested(uint)), SLOT(Error()));
}

void Main::Quit()
{ QCoreApplication::quit(); }

void Main::NameRequested(uint code)
{ 
    qDebug("NameRequest: %u", code);
    this->deleteLater();
}

void Main::Error()
{ qDebug("Error"); }

static void Log(const char* str, size_t sz)
{ qDebug("%.*s", (int) sz, str); }

int main(int argc, char* argv[])
{
    adbus_set_logger(&Log);
    QCoreApplication app(argc, argv);

    QDBusConnection c = QDBusConnection::sessionBus();

    if (!c.isConnected()) {
        qFatal("Can't connect");
        return 1;
    }

    qDebug() << "Connected as" << c.baseService();

    new Main(c);

    return app.exec();
}

