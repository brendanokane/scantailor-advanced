/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PAGE_LAYOUT_ALIGNMENT_H_
#define PAGE_LAYOUT_ALIGNMENT_H_

class QDomDocument;
class QDomElement;
class QString;

#include <iostream>

class CommandLine;

namespace page_layout {
    const double DEFAULT_TOLERANCE = 0.2;

    class Alignment {
    public:
        enum Vertical {
            TOP,
            VCENTER,
            BOTTOM,
            VAUTO,
            VORIGINAL
        };

        enum Horizontal {
            LEFT,
            HCENTER,
            RIGHT,
            HAUTO,
            HORIGINAL
        };

        /**
         * \brief Constructs a null alignment.
         */
        Alignment();

        Alignment(Vertical vert, Horizontal hor);

        explicit Alignment(const QDomElement& el);

        Vertical vertical() const;

        void setVertical(Vertical vert);

        Horizontal horizontal() const;

        void setHorizontal(Horizontal hor);

        bool isNull() const;

        void setNull(bool is_null);

        double tolerance() const;

        void setTolerance(double t);

        bool operator==(const Alignment& other) const;

        bool operator!=(const Alignment& other) const;

        QDomElement toXml(QDomDocument& doc, const QString& name) const;

    private:
        Vertical m_vert;
        Horizontal m_hor;
        bool m_isNull;
        double m_tolerance;
    };
}  // namespace page_layout
#endif  // ifndef PAGE_LAYOUT_ALIGNMENT_H_
