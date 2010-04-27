//  
//  Copyright (C) 2009 Canonical Ltd.
// 
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
// 
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
// 
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
// 

#ifndef __BAMFMATCHER_H__
#define __BAMFMATCHER_H__

#include "bamf.h"
#include <glib.h>
#include <glib-object.h>

#define BAMF_TYPE_MATCHER			(bamf_matcher_get_type ())
#define BAMF_MATCHER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BAMF_TYPE_MATCHER, BamfMatcher))
#define BAMF_IS_MATCHER				(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAMF_TYPE_MATCHER))
#define BAMF_MATCHER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BAMF_TYPE_MATCHER, BamfMatcherClass))
#define BAMF_IS_MATCHER_CLASS(klass)		(G_TYPE_CHECK_CLASA_TYPE ((klass), BAMF_TYPE_MATCHER))
#define BAMF_MATCHER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), BAMF_TYPE_MATCHER, BamfMatcherClass))

typedef struct _BamfMatcher BamfMatcher;
typedef struct _BamfMatcherClass BamfMatcherClass;
typedef struct _BamfMatcherPrivate BamfMatcherPrivate;

struct _BamfMatcherClass
{
  GObjectClass parent;
};

struct _BamfMatcher
{
  GObject parent;

  /* private */
  BamfMatcherPrivate *priv;
};

GType bamf_matcher_get_type (void) G_GNUC_CONST;

char *
bamf_matcher_application_for_xid (BamfMatcher *matcher,
                                  guint32 xid);

gboolean
bamf_matcher_application_is_running (BamfMatcher *matcher,
                                     char *application);

GArray *
bamf_matcher_application_dbus_paths (BamfMatcher *matcher);

char *
bamf_matcher_dbus_path_for_application (BamfMatcher *matcher,
                                        char *application);

GArray *
bamf_matcher_running_application_paths (BamfMatcher *matcher);

GArray *
bamf_matcher_tab_dbus_paths (BamfMatcher *matcher);

GArray *
bamf_matcher_xids_for_application (BamfMatcher *matcher,
                                   char *application);

BamfMatcher * bamf_matcher_get_default (void);

#endif