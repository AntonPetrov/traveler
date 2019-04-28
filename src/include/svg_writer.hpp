/*
 * File: svg_writer.hpp
 *
 * Copyright (C) 2016 Richard Eliáš <richard.elias@matfyz.cz>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

#ifndef SVG_WRITER_HPP
#define SVG_WRITER_HPP

#include "document_writer.hpp"

class svg_writer : public document_writer
{
private:
    struct properties;
    struct style;
    
public:
    virtual ~svg_writer() = default;
    
public:
    virtual void init(
                      const std::string& filename,
                      rna_tree& rna);
    virtual streampos print(
                            const std::string& text);
    
public: // formatters
    virtual std::string get_circle_formatted(
                                             point centre,
                                             double radius) const;
    virtual std::string get_label_formatted(
                                            const rna_label& label,
                                            const RGB& color,
                                            const label_info li) const;
    
protected:
    virtual std::string get_line_formatted(
                                           point from,
                                           point to,
                                           const RGB& color) const;

//    double get_scaling_ratio() const;
    
private:
    std::string get_header_element(
                                   rna_tree::iterator root);
    style get_color_style(
                          const RGB& color) const;
    properties get_styles(
                          const std::vector<style>& styles) const;
    std::string create_element(
                               const std::string& name,
                               const properties& properties,
                               const std::string& value = "",
                               const label_info li = {-1, ""}) const;
//    std::string create_element(
//            const std::string& name,
//            const properties& properties,
//            const int ix) const;
    std::string create_style_definitions() const;
    
    properties get_point_formatted(
                                   point p,
                                   const std::string& prefix,
                                   const std::string& postfix,
                                   bool should_shift_p = true) const;

    void scale_point(point &p) const;
    
private:
    point dimensions;
    point margin;

    point shift;
//    point scale;
    point tr, bl;
    point letter;

};

#endif /* !SVG_WRITER_HPP */
