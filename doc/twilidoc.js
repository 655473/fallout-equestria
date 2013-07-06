// Generated by CoffeeScript 1.3.3
(function() {
  var ATTR_CONST, ATTR_INLINE, ATTR_PTR, ATTR_REF, ATTR_STATIC, ATTR_TYPEDEF, ATTR_UNSIGNED, ATTR_VIRTUAL, AnchorHandle, Attribute, Breadcrumb, Class, ClassList, Foldables, Homepage, Member, Menu, MenuEntry, Project, String, View, VisibilityHandle, Widget, escape_html,
    __hasProp = {}.hasOwnProperty,
    __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

  ATTR_PTR = 1;

  ATTR_REF = 2;

  ATTR_CONST = 4;

  ATTR_UNSIGNED = 8;

  ATTR_STATIC = 16;

  ATTR_INLINE = 32;

  ATTR_VIRTUAL = 64;

  ATTR_TYPEDEF = 128;

  escape_html = function(text) {
    var result;
    result = text;
    result = result.replace(/</g, '&lt;');
    return result = result.replace(/>/g, '&gt;');
  };

  String = (function() {

    function String(str) {
      this.string = str;
    }

    String.prototype.Substr = function(i, end) {
      var result;
      end = end != null ? end + i : this.string.length;
      result = '';
      while (i < this.string.length && i < end) {
        result += this.string[i];
        i++;
      }
      return result;
    };

    String.prototype.Split = function(sep) {
      var i, last_part, parts, to_comp;
      i = 0;
      parts = [];
      last_part = 0;
      while (i + sep.length < this.string.length) {
        to_comp = this.Substr(i, sep.length);
        if (to_comp === sep) {
          parts.push(this.Substr(last_part, i - last_part));
          last_part = i + sep.length;
          i = last_part;
          continue;
        }
        i++;
      }
      if (last_part !== i) {
        parts.push(this.Substr(last_part, this.string.length));
      }
      return parts;
    };

    return String;

  })();

  window.String = String;

  Array.prototype.Join = function(sep) {
    var item, string, _i, _len;
    string = '';
    for (_i = 0, _len = this.length; _i < _len; _i++) {
      item = this[_i];
      string += item;
    }
    return string;
  };

  Array.prototype.Includes = function(item) {
    return (this.indexOf(item)) !== -1;
  };

  Project = (function() {
    var CandidatesFromType;

    function Project() {}

    Project.prototype.GetTypedef = function(name, parent) {
      var candidates, typedef, _i, _len, _ref;
      candidates = CandidatesFromType(name, parent);
      _ref = project.typedefs;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        typedef = _ref[_i];
        if (typeof candidates.Includes === "function" ? candidates.Includes(typedef.name) : void 0) {
          return typedef;
        }
      }
      return null;
    };

    Project.prototype.GetType = function(name, parent) {
      var candidates, type, _i, _len, _ref;
      if (name.string != null) {
        name = name.string;
      }
      if ((name.match(/</)) != null) {
        name = new String(name);
        name = (name.Split('<'))[0];
      }
      candidates = CandidatesFromType(name, parent);
      if (candidates.length > 0) {
        console.log(candidates);
        _ref = project.types;
        for (_i = 0, _len = _ref.length; _i < _len; _i++) {
          type = _ref[_i];
          if (type.name == null) {
            continue;
          }
          if (candidates.Includes(type.name)) {
            return type;
          }
        }
      }
      return null;
    };

    CandidatesFromType = function(name, parent) {
      var candidates, i, merge, part, parts, _i, _len;
      if ((parent != null) && (name != null)) {
        parent = new String(parent);
        parts = parent.Split('::');
        candidates = [];
        for (_i = 0, _len = parts.length; _i < _len; _i++) {
          part = parts[_i];
          merge = (candidates.Join('::')) + part;
          candidates.push(merge);
        }
        i = 0;
        while (i < candidates.length) {
          candidates[i++] += '::' + name;
        }
        candidates.push(name);
        return candidates;
      } else if (name != null) {
        return [name];
      } else {
        return [];
      }
    };

    return Project;

  })();

  View = (function() {

    function View() {}

    View.prototype.AfterFilter = function() {
      this.RefreshVisibility();
      this.InitRoutables();
      return (this.elem.find('[data-rel="popover"]')).popover({
        trigger: 'hover'
      });
    };

    View.prototype.Display = function() {
      var _this = this;
      return $("#page-content").fadeOut(function() {
        $("#page-content").empty();
        $("#page-content").append(_this.elem);
        $("#page-content").fadeIn(function() {
          return _this.AfterFilter();
        });
        if (window.current_view != null) {
          window.current_view.Destroy();
        }
        return window.current_view = _this;
      });
    };

    View.prototype.RefreshVisibility = function() {
      var private_elems, protected_elems, public_elems;
      public_elems = this.elem.find('.visibility-public');
      console.log(public_elems);
      protected_elems = this.elem.find('.visibility-protected');
      private_elems = this.elem.find('.visibility-private');
      if (window.vision_handle.DisplayPublic()) {
        public_elems.show();
      } else {
        public_elems.hide();
      }
      if (window.vision_handle.DisplayProtected()) {
        protected_elems.show();
      } else {
        protected_elems.hide();
      }
      if (window.vision_handle.DisplayPrivate()) {
        return private_elems.show();
      } else {
        return private_elems.hide();
      }
    };

    View.prototype.Destroy = function() {
      return $(".popover").fadeOut(function() {
        return $(this).remove();
      });
    };

    View.prototype.InitRoutables = function() {
      var routables;
      routables = this.elem.find('[data-route]');
      return routables.each(function() {
        return $(this).click(function() {
          return location.hash = $(this).attr('data-route');
        });
      });
    };

    return View;

  })();

  Widget = (function() {

    function Widget() {}

    Widget.prototype.Begin = function(title, icon, controls) {
      return "<div class='row-fluid sortable'>        <div class='box span12'>          <div class='box-header well' data-original-title>            <h2>" + (icon != null ? "<i class='" + icon + "'></i>" : '') + (" " + title + "</h2>            <div class='box-icon'>            ") + (controls != null ? controls : '') + "            </div>          </div>          <div class='box-content'>";
    };

    Widget.prototype.End = function() {
      return "</div></div></div>";
    };

    return Widget;

  })();

  Attribute = (function() {

    function Attribute() {}

    Attribute.prototype.TypeBox = function(name, attrs, type, is_typedef) {
      var classname, desc, html, klass;
      html = '';
      html += "<p class='btn-group'>";
      if (attrs & ATTR_PTR) {
        html += "<button class='btn btn-mini btn-info'>ptr</button>";
      }
      if (attrs & ATTR_REF) {
        html += "<button class='btn btn-mini btn-info'>ref</button>";
      }
      if (attrs & ATTR_CONST) {
        html += "<button class='btn btn-mini'>const</button>";
      }
      if (attrs & ATTR_UNSIGNED) {
        html += "<button class='btn btn-mini'>unsigned</button>";
      }
      name = (name.replace(/</g, '[')).replace(/>/g, ']');
      if ((type != null) || is_typedef === true) {
        klass = is_typedef === true ? 'btn-warning' : 'btn-success';
        html += "<button class='btn btn-mini " + klass + "'";
        if (!(is_typedef && type === null)) {
          html += " data-route='#show-class-" + type.name + "'";
        }
        desc = type === null || !(type.doc != null) ? 'Undocumented type' : type.doc.overview.replace("'", "&#39;");
        classname = type != null ? type.name : name;
        classname = (classname.replace(/</g, '[')).replace(/>/g, ']');
        html += "data-rel='popover' data-content='" + desc + "' title='" + classname + "'";
        html += ">" + name + "</button>";
      } else {
        html += "<button class='btn btn-mini btn-inverse'>" + name + "</button>";
      }
      html += "</p>";
      return html;
    };

    Attribute.prototype.Attribufy = function(html) {
      return html.replace(/\[([a-z0-9_]+)\]/ig, function(match, content, offset, s) {
        var type;
        type = Project.prototype.GetType(content, null);
        return Attribute.prototype.TypeBox(content, 0, type, false);
      });
    };

    return Attribute;

  })();

  Member = (function(_super) {

    __extends(Member, _super);

    function Member(classname, type, element) {
      var html, icon;
      icon = '';
      if (type === 'method') {
        icon = 'icon-cog';
      }
      if (type === 'attribute') {
        icon = 'icon-asterisk';
      }
      html = Widget.prototype.Begin("" + classname + "::" + element.name, icon);
      if (type === 'method') {
        html += this.RenderMethod(element, classname);
      } else if (type === 'attribute') {
        html += this.RenderAttribute(element, classname);
      } else {
        alert("Unimplemented type " + type + " for Member View");
        throw "Unimplemented type " + type + " for Member View";
      }
      html += Widget.prototype.End();
      this.elem = $(html);
    }

    Member.prototype.AfterFilter = function() {
      Member.__super__.AfterFilter.apply(this, arguments);
      return sh_highlightDocument();
    };

    Member.prototype.RenderMethod = function(method, classname) {
      var html, name, params, return_type, template_parts, typedef, visibility, _typedef;
      html = '';
      html += "<div class='span12' style='margin-top: 10px;'><div>";
      html += "<div class='method-descriptor'>";
      html += "<span class='span3'>";
      if (method.return_type != null) {
        name = new String(method.return_type);
        template_parts = name.Split('<');
        if (template_parts.length > 1) {
          name = template_parts[0];
        }
        console.log("Classname is: " + classname + "!");
        return_type = Project.prototype.GetType(name, classname);
        if (!(return_type != null)) {
          typedef = Project.prototype.GetTypedef(name, classname);
          _typedef = typedef;
          while ((!(return_type != null)) && (_typedef != null)) {
            return_type = Project.prototype.GetType(_typedef.to, classname);
            if (!(return_type != null)) {
              _typedef = Project.prototype.GetTypedef(_typedef.to, classname);
            }
          }
          if (typedef != null) {
            if (return_type != null) {
              html += Attribute.prototype.TypeBox(typedef.name, method.return_attrs, return_type, true);
            } else {
              html += Attribute.prototype.TypeBox(typedef.name, method.return_attrs, {
                name: typedef.to
              }, true);
            }
          } else {
            html += Attribute.prototype.TypeBox(method.return_type, method.return_attrs);
          }
        } else if (return_type != null) {
          html += Attribute.prototype.TypeBox(method.return_type, method.return_attrs, return_type);
        }
      } else {
        if (method.name[0] !== '~') {
          html += '<span class="label label-info"><i class="icon-cogs"></i> Constructor</span>';
        } else {
          html += '<span class="label label-info"><i class="icon-trash"></i> Destructor</span>';
        }
      }
      visibility = {
        "class": '<span class="label label-warning"><i class="icon-tag"></i> Class</span>',
        object: '<span class="label label-primary"><i class="icon-tag"></i> Object</span>',
        virtual: '<span class="label label-inverse"><i class="icon-random"></i> Virtual</span>'
      };
      visibility = method.attrs & ATTR_STATIC ? visibility["class"] : visibility.object;
      if (method.attrs & ATTR_CONST) {
        visibility = '<span class="label label-info"><i class="icon-ban-circle"></i> Const</span> ' + visibility;
      }
      if (method.attrs & ATTR_VIRTUAL) {
        visibility = '<span class="label label-inverse"><i class="icon-random"></i> Virtual</span> ' + visibility;
      }
      params = escape_html(method.params);
      html += "</span>";
      html += "<div class='span9'>";
      html += "<span class='span3'><h4>" + method.name + "</h4></span>";
      html += "<span class='span6'><pre>" + params + "</pre></span>";
      html += "<span class='span3'><div style='float:right;'>" + visibility + "</div></span>";
      html += "</div>";
      if (method.doc != null) {
        html += this.RenderDoc(method);
      }
      html += "</div>";
      html += "</div>";
      html += "</div>";
      html = html.replace('<pre>', '<pre class="sh_cpp">');
      return html;
    };

    Member.prototype.RenderAttribute = function(attribute, classname) {
      var html, obj_type, typedef, visibility, _typedef;
      html = '';
      obj_type = Project.prototype.GetType(attribute.type, classname);
      html += "<div class='span12' style='margin-top: 10px;'><div>";
      html += "<div class='attribute-descriptor'>";
      html += "<span class='span3'>";
      if (!(obj_type != null)) {
        typedef = Project.prototype.GetTypedef(attribute.type, classname);
        _typedef = typedef;
        while ((!(obj_type != null)) && (_typedef != null)) {
          obj_type = Project.prototype.GetType(_typedef.to, classname);
          if (!(obj_type != null)) {
            _typedef = Project.prototype.GetTypedef(_typedef.to, classname);
          }
        }
        if (typedef != null) {
          if (obj_type != null) {
            html += Attribute.prototype.TypeBox(typedef.name, attribute.attrs, obj_type, true);
          } else {
            html += Attribute.prototype.TypeBox(typedef.name, attribute.attrs, {
              name: typedef.to
            }, true);
          }
        } else {
          html += Attribute.prototype.TypeBox(attribute.type, attribute.attrs);
        }
      } else {
        html += Attribute.prototype.TypeBox(attribute.type, attribute.attrs, obj_type);
      }
      html += "</span>";
      html += "<span class='span6'>";
      html += "<h4>" + attribute.name + "</h4>";
      if (attribute.doc != null) {
        html += this.RenderDoc(attribute);
      }
      html += "</span>";
      visibility = {
        "class": '<span class="label label-warning"><i class="icon-tag"></i> Class</span>',
        object: '<span class="label label-primary"><i class="icon-tag"></i> Object</span>'
      };
      visibility = attribute.attrs & ATTR_STATIC ? visibility["class"] : visibility.object;
      html += "<span class='span3'><div style='float:right;'>" + visibility + "</div></span>";
      html += "</div>";
      html += "</div></div>";
      return html;
    };

    Member.prototype.RenderDoc = function(attribute) {
      var doc, has_doc;
      has_doc = false;
      doc = '';
      doc += '<div class="well foldable" style="padding: 0 0 0 0; margin: 0 0 0 0; padding-left: 5px;"><dl>';
      if ((attribute.doc.short != null) && attribute.doc.short !== '') {
        doc += "<dt>Overview</dt><dd>" + attribute.doc.short + "</dd>";
        has_doc = true;
      }
      if ((attribute.doc.desc != null) && attribute.doc.desc !== '') {
        doc += "<dt>Details</dt><dd>" + attribute.doc.desc + "</dd>";
        has_doc = true;
      }
      doc += '</dl></div>';
      doc = doc.replace('<pre>', '<pre class="sh_cpp">');
      if (has_doc) {
        return doc;
      } else {
        return '';
      }
    };

    return Member;

  })(View);

  Class = (function(_super) {

    __extends(Class, _super);

    function Class(classname) {
      var attribute, html, method, obj_type, type, _i, _j, _k, _len, _len1, _len2, _ref, _ref1, _ref2;
      this.view_type = 'class';
      _ref = project.types;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        type = _ref[_i];
        if (type.name === classname) {
          this.type = type;
          break;
        }
      }
      if (this.type == null) {
        alert("Type " + classname + " not found.");
        throw "Type " + classname + " not found.";
      }
      html = Widget.prototype.Begin("" + type.decl + " " + type.name, 'icon-list-alt');
      html += "<div id='uml'></div>";
      html += Widget.prototype.End();
      if (this.type.methods.length > 0) {
        html += Widget.prototype.Begin("Methods", "icon-cog");
        html += "<div class='span12'></div>";
        _ref1 = this.type.methods;
        for (_j = 0, _len1 = _ref1.length; _j < _len1; _j++) {
          method = _ref1[_j];
          html += "<div class='visibility-" + method.visibility + "'>";
          html += Member.prototype.RenderMethod(method, classname);
          html += "</div>";
        }
        html += Widget.prototype.End();
      }
      if (this.type.attributes.length > 0) {
        html += Widget.prototype.Begin("Attributes", "icon-asterisk");
        html += "<div class='span12'></div>";
        _ref2 = this.type.attributes;
        for (_k = 0, _len2 = _ref2.length; _k < _len2; _k++) {
          attribute = _ref2[_k];
          html += "<div class='visibility-" + attribute.visibility + "'>";
          html += Member.prototype.RenderAttribute(attribute, classname);
          html += "</div>";
          obj_type = null;
        }
        html += Widget.prototype.End();
      }
      $.ajax({
        url: "docs/" + classname + ".html",
        dataType: 'html',
        async: false,
        success: function(data) {
          html += Widget.prototype.Begin("Documentation", "icon-file");
          html += data;
          return html += Widget.prototype.End();
        }
      });
      this.elem = $(html);
    }

    Class.prototype.AfterFilter = function() {
      Class.__super__.AfterFilter.apply(this, arguments);
      sh_highlightDocument();
      return window.uml.generate_hierarchy('uml', this.type.name);
    };

    return Class;

  })(View);

  ClassList = (function(_super) {

    __extends(ClassList, _super);

    function ClassList(object_types) {
      var html, i, title, type, _i, _len, _ref, _ref1;
      if (object_types == null) {
        object_types = ['class', 'struct'];
      }
      title = object_types.Includes('class') ? 'Class Index' : 'Namespace Index';
      html = Widget.prototype.Begin(title, 'icon-list');
      html += "<table class='table table-striped table-bordered bootstrap-datatable datatable dataTable'>";
      html += "<thead>";
      html += "<tr role='row'>";
      html += "<th>File</th>";
      html += "<th>Name</th>";
      html += "<th>Overview</th>";
      html += "<th>Actions</th>";
      html += "</tr>";
      html += "</thead>";
      html += "<tbody role='alert' aria-live='polite' aria-relevant='all'>";
      i = 0;
      _ref = project.types;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        type = _ref[_i];
        if (!object_types.Includes(type.decl)) {
          continue;
        }
        html += "<tr class='" + ((_ref1 = i % 2 === 0) != null ? _ref1 : {
          'even': 'odd'
        }) + "'>";
        html += "<td>" + type.file + "</td>";
        html += "<td>" + type.name + "</td>";
        html += "<td>";
        if ((type.doc != null) && type.doc.overview !== '') {
          html += type.doc.overview.length > 40 ? type.doc.overview.slice(0, 41) + "..." : type.doc.overview;
        }
        html += "</td>";
        html += "<td>";
        html += "<a class='btn btn-success' href='#show-" + object_types[0] + "-" + type.name + "'>";
        html += "<i class='icon-zoom-in icon-white' /> View";
        html += "</a>";
        if ((type.doc != null) && type.doc.overview !== '') {
          html += "<button style='margin-left: 2px;' class='btn btn-primary' data-rel='popover' data-content='" + type.doc.overview + "' title='" + type.name + "'><i class='icon-eye-open'></i> Overview</button>";
        } else {
          html += "<button style='margin-left: 2px;' class='btn btn-primary disabled'><i class='icon-eye-open'></i> Overview</button>";
        }
        html += "</td>";
        html += "</tr>";
      }
      html += "</tbody>";
      html += "</table>";
      html += Widget.prototype.End();
      this.elem = $(html);
      this.table = this.elem.find('table');
    }

    ClassList.prototype.AfterFilter = function() {
      var update_popovers,
        _this = this;
      update_popovers = function() {
        return (_this.elem.find('[data-rel="popover"]')).popover({
          trigger: 'click',
          placement: 'left'
        });
      };
      this.table.dataTable().fnDestroy();
      return this.table.dataTable({
        "sDom": "<'row-fluid'<'span6'l><'span6'f>r>t<'row-fluid'<'span12'i><'span12 center'p>>",
        "sPaginationType": "bootstrap",
        "oLanguage": {
          "sLengthMenu": "_MENU_ records per page"
        },
        "fnDrawCallback": update_popovers
      });
    };

    return ClassList;

  })(View);

  Homepage = (function(_super) {

    __extends(Homepage, _super);

    function Homepage() {
      var html;
      html = Widget.prototype.Begin("" + project.name);
      html += project.desc.homepage;
      html += Widget.prototype.End();
      html = Attribute.prototype.Attribufy(html);
      this.elem = $(html);
    }

    Homepage.prototype.AfterFilter = function() {
      Homepage.__super__.AfterFilter.apply(this, arguments);
      return sh_highlightDocument();
    };

    return Homepage;

  })(View);

  MenuEntry = (function() {

    function MenuEntry(menu, id, desc) {
      var disp_name, html, icon, title;
      icon = desc.type === 'search' ? 'icon-eye-open' : desc.type === 'class' ? 'icon-list-alt' : desc.type === 'method' ? 'icon-cog' : desc.type === 'attribute' ? 'icon-asterisk' : void 0;
      this.menu = menu;
      disp_name = desc.name;
      title = '';
      if (disp_name.length >= 21) {
        disp_name = disp_name.slice(0, 17) + '...';
        title = " title='" + desc.name + "' data-rel='tooltip'";
      }
      html = "<li id='" + id + "'>";
      html += "<a class='ajax-link' href='" + desc.url + "' " + title + "><i class='" + icon + "'></i>";
      html += "<span class='hidden-tablet'> " + disp_name + "</span>";
      html += '</a></li>';
      this.elem = $(html);
      this.menu.results.append(this.elem);
    }

    return MenuEntry;

  })();

  Menu = (function() {

    function Menu() {
      var _this = this;
      this.dom = $('#main-menu');
      this.search = this.dom.find('#menu-search');
      this.search = $("#main-menu").find("#menu-search");
      this.results = $("#main-menu").find("#menu-results");
      this.entries = [];
      this.search.keyup(function() {
        return _this.SearchUpdate();
      });
    }

    Menu.prototype.MaxElems = function() {
      var entry, height;
      height = $(window).height() - 45 - (74 - 18) - 200;
      entry = 18 * 2;
      return (height / entry) - 1;
    };

    Menu.prototype.SearchUpdate = function() {
      var attribute, max_count, method, regex, res_count, type, _i, _j, _k, _len, _len1, _len2, _ref, _ref1, _ref2;
      regex = new RegExp(this.search.attr('value'));
      this.ClearResults();
      if ((this.search.attr('value')) === '') {
        return;
      }
      res_count = 0;
      max_count = this.MaxElems();
      _ref = project.types;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        type = _ref[_i];
        if (type.name.match(regex)) {
          this.AddMenuEntry(type.name, {
            name: type.name,
            url: "#show-class-" + type.name,
            type: 'class'
          });
          res_count++;
        }
        if (res_count > max_count) {
          break;
        }
        _ref1 = type.methods;
        for (_j = 0, _len1 = _ref1.length; _j < _len1; _j++) {
          method = _ref1[_j];
          if (method.name.match(regex)) {
            this.AddMenuEntry(method.name, {
              name: "" + type.name + "::" + method.name,
              url: "#show-class-" + type.name + "-method-" + method.name,
              type: 'method'
            });
            res_count++;
          }
        }
        if (res_count > max_count) {
          break;
        }
        _ref2 = type.attributes;
        for (_k = 0, _len2 = _ref2.length; _k < _len2; _k++) {
          attribute = _ref2[_k];
          if (attribute.name.match(regex)) {
            this.AddMenuEntry(attribute.name, {
              name: "" + type.name + "::" + attribute.name,
              url: "#show-class-" + type.name + "-attribute-" + attribute.name,
              type: 'attribute'
            });
            res_count++;
          }
        }
        if (res_count > max_count) {
          break;
        }
      }
      if (res_count > max_count) {
        this.AddMenuEntry('more-results', {
          name: 'More results...',
          url: '#more-results',
          type: 'search'
        });
      }
      return (this.dom.find('[data-rel="tooltip"]')).tooltip({
        placement: 'right'
      });
    };

    Menu.prototype.AddMenuEntry = function(id, desc) {
      var entry;
      entry = new MenuEntry(this, id, desc);
      return this.entries.push(entry);
    };

    Menu.prototype.ClearResults = function() {
      this.results.empty();
      return this.entries = [];
    };

    return Menu;

  })();

  Breadcrumb = (function() {

    function Breadcrumb() {
      console.log('Breadcrumb');
      this.dom = $('#breadcrumb');
      this.loading_handle = $("#loading-indicator");
    }

    Breadcrumb.prototype.Clear = function() {
      this.dom.empty();
      return this;
    };

    Breadcrumb.prototype.Add = function(name, url) {
      var html;
      (this.dom.find('li')).last().append("<span class='divider'>/</span>");
      html = "<li>";
      if (url != null) {
        html += "<a href='" + url + "'>";
      }
      html += name;
      if (url != null) {
        html += '</a>';
      }
      html += "</li>";
      this.dom.append(html);
      return this;
    };

    Breadcrumb.prototype.PrefabNamespaces = function() {
      return this.Clear().Add('Project').Add('Namespaces', '#namespaces-index');
    };

    Breadcrumb.prototype.PrefabClasses = function() {
      return this.Clear().Add('Project').Add('Class List', '#class-index');
    };

    Breadcrumb.prototype.PrefabClass = function(class_name) {
      return this.PrefabClasses().Add('Class').Add(class_name, "#show-class-" + class_name);
    };

    Breadcrumb.prototype.SetLoading = function(set) {
      if (set) {
        return this.loading_handle.fadeIn();
      } else {
        return this.loading_handle.fadeOut();
      }
    };

    return Breadcrumb;

  })();

  Foldables = (function() {

    function Foldables() {
      var _this = this;
      this.control = {
        fold_all: $('#fold-all'),
        unfold_all: $('#unfold-all')
      };
      this.control.fold_all.click(function() {
        return _this.SetVisible(false);
      });
      this.control.unfold_all.click(function() {
        return _this.SetVisible(true);
      });
    }

    Foldables.prototype.SetVisible = function(set_visible) {
      if (set_visible) {
        return $('.foldable').slideDown();
      } else {
        return $('.foldable').slideUp();
      }
    };

    return Foldables;

  })();

  VisibilityHandle = (function() {
    var PRIVATE, PROTECTED, PUBLIC;

    PUBLIC = 1;

    PROTECTED = 2;

    PRIVATE = 4;

    function VisibilityHandle(default_val) {
      var _this = this;
      this.base = $('#visibility-filter');
      this.flag = 0;
      this.toggle_public = this.base.find('.visibility-public');
      this.toggle_protected = this.base.find('.visibility-protected');
      this.toggle_private = this.base.find('.visibility-private');
      this.toggle_public.click(function() {
        return _this.DisplayPublic(!(_this.DisplayPublic()));
      });
      this.toggle_protected.click(function() {
        return _this.DisplayProtected(!(_this.DisplayProtected()));
      });
      this.toggle_private.click(function() {
        return _this.DisplayPrivate(!(_this.DisplayPrivate()));
      });
      this.toggle_public.click(function() {
        return _this.Refresh();
      });
      this.toggle_protected.click(function() {
        return _this.Refresh();
      });
      this.toggle_private.click(function() {
        return _this.Refresh();
      });
      if ((default_val & PUBLIC) !== 0) {
        this.toggle_public.click();
      }
      if ((default_val & PROTECTED) !== 0) {
        this.toggle_protected.click();
      }
      if ((default_val & PRIVATE) !== 0) {
        this.toggle_private.click();
      }
    }

    VisibilityHandle.prototype.DisplayPublic = function(param) {
      return this.FlagGetSet(PUBLIC, param);
    };

    VisibilityHandle.prototype.DisplayProtected = function(param) {
      return this.FlagGetSet(PROTECTED, param);
    };

    VisibilityHandle.prototype.DisplayPrivate = function(param) {
      return this.FlagGetSet(PRIVATE, param);
    };

    VisibilityHandle.prototype.FlagGetSet = function(value, param) {
      if (param != null) {
        if (param === true && (this.flag & value) === 0) {
          this.flag += value;
        } else if (param === false && (this.flag & value) !== 0) {
          this.flag -= value;
        }
      }
      return this.flag & value;
    };

    VisibilityHandle.prototype.Refresh = function() {
      if (window.current_view != null) {
        return window.current_view.RefreshVisibility();
      }
    };

    return VisibilityHandle;

  })();

  AnchorHandle = (function() {

    function AnchorHandle() {
      var callback,
        _this = this;
      callback = function() {
        return _this.Refresh();
      };
      setInterval(callback, 50);
      this.anchor = location.hash;
      this.routes = [];
    }

    AnchorHandle.prototype.AddRoute = function(regex, callback) {
      return this.routes.push({
        exp: regex,
        callback: callback
      });
    };

    AnchorHandle.prototype.Refresh = function() {
      if (this.anchor !== location.hash) {
        this.anchor = location.hash;
        return this.Execute();
      }
    };

    AnchorHandle.prototype.Execute = function() {
      var route, _i, _len, _ref;
      console.log('Execute anchor path matching');
      _ref = this.routes;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        route = _ref[_i];
        if (this.anchor.match(route.exp)) {
          console.log('Found matching route');
          window.breadcrumb.SetLoading(true);
          route.callback(this.anchor);
          window.breadcrumb.SetLoading(false);
          return;
        }
      }
      return alert('Not Found');
    };

    return AnchorHandle;

  })();

  window.project_chart = function() {
    var data, meth_count, meth_counted, type, _i, _j, _len, _len1, _ref, _ref1;
    meth_count = 0;
    meth_counted = 0;
    data = [];
    _ref = project.types;
    for (_i = 0, _len = _ref.length; _i < _len; _i++) {
      type = _ref[_i];
      meth_count += type.methods.length;
    }
    _ref1 = project.types;
    for (_j = 0, _len1 = _ref1.length; _j < _len1; _j++) {
      type = _ref1[_j];
      if (type.methods.length > meth_count / 75) {
        data.push({
          label: type.name,
          data: type.methods.length
        });
      }
    }
    $.plot($("#piechart"), data, {
      series: {
        pie: {
          show: true
        }
      },
      grid: {
        hoverable: true,
        clickable: true
      },
      legend: {
        show: false
      }
    });
    return $("#piechart").bind("plothover", pieHover);
  };

  $(document).ready(function() {
    console.log('[Twilidoc] Initializing');
    window.menu = new Menu();
    window.breadcrumb = new Breadcrumb();
    window.anchor_handle = new AnchorHandle();
    window.vision_handle = new VisibilityHandle(1);
    window.foldables = new Foldables();
    window.anchor_handle.AddRoute(/^$/, function() {
      return console.log('Nothing');
    });
    window.anchor_handle.AddRoute(/#project/, function() {
      if (window.homepage == null) {
        window.homepage = new Homepage();
      }
      window.homepage.Display();
      return window.breadcrumb.Clear().Add('Project');
    });
    window.anchor_handle.AddRoute(/#class-index/, function() {
      console.log('anchor handler executed');
      if (window.class_list == null) {
        window.class_list = new ClassList();
      }
      window.class_list.Display();
      return window.breadcrumb.PrefabClasses();
    });
    window.anchor_handle.AddRoute(/#namespaces-index/, function() {
      if (window.namespaces_list == null) {
        window.namespaces_list = new ClassList(['namespace']);
      }
      window.namespaces_list.Display();
      return window.breadcrumb.PrefabNamespaces();
    });
    window.anchor_handle.AddRoute(/#show-class-[^-]+$/, function() {
      var class_view, matches;
      matches = /#show-class-([^-]+)$/.exec(window.anchor_handle.anchor);
      class_view = new Class(matches[1]);
      class_view.Display();
      return window.breadcrumb.PrefabClass(matches[1]);
    });
    window.anchor_handle.AddRoute(/#show-class-[^-]+-method-[^-]+$/, function() {
      var item, klass, matches, method, obj_meth, obj_type, view, _i, _len, _ref;
      matches = /#show-class-([^-]+)-method-([^-]+)$/.exec(window.anchor_handle.anchor);
      klass = matches[1];
      method = matches[2];
      if ((window.current_view != null) && window.current_view.view_type === 'class' && window.current_view.type.name === klass) {
        return alert('Opening a method in an already opened class is not implement yet :(');
      } else {
        window.breadcrumb.PrefabClass(matches[1]).Add('Methods').Add(matches[2], "#show-class-" + matches[1] + "-method-" + matches[2]);
        obj_type = window.get_project_type(klass);
        obj_meth = null;
        _ref = obj_type.methods;
        for (_i = 0, _len = _ref.length; _i < _len; _i++) {
          item = _ref[_i];
          if (item.name === method) {
            obj_meth = item;
            break;
          }
        }
        view = new Member(klass, 'method', obj_meth);
        return view.Display();
      }
    });
    window.anchor_handle.AddRoute(/#show-class-[^-]+-attribute-[^-]+$/, function() {
      var attr, item, klass, matches, obj_attr, obj_type, view, _i, _len, _ref;
      matches = /#show-class-([^-]+)-attribute-([^-]+)$/.exec(window.anchor_handle.anchor);
      klass = matches[1];
      attr = matches[2];
      if ((window.current_view != null) && window.current_view.view_type === 'class' && window.current_view.type.name === klass) {
        return alert('Opening an attribute in an already opened class is not implement yet :(');
      } else {
        window.breadcrumb.PrefabClass(matches[1]).Add('Attributes').Add(matches[2], "#show-class-" + matches[1] + "-attribute-" + matches[2]);
        obj_type = window.get_project_type(klass);
        obj_attr = null;
        _ref = obj_type.attributes;
        for (_i = 0, _len = _ref.length; _i < _len; _i++) {
          item = _ref[_i];
          if (item.name === attr) {
            obj_attr = item;
            break;
          }
        }
        view = new Member(klass, 'attribute', obj_attr);
        return view.Display();
      }
    });
    window.anchor_handle.anchor = '#project';
    window.anchor_handle.Execute();
    return console.log('[Twilidoc] Finished initializing');
  });

}).call(this);
