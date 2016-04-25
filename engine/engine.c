/**
  The MIT License (MIT)

  Copyright (c) Navaneeth K.N

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
**/

#include <string.h>
#include <varnam.h>
#include "engine.h"

typedef struct _IBusVarnamEngine IBusVarnamEngine;
typedef struct _IBusVarnamEngineClass IBusVarnamEngineClass;

struct _IBusVarnamEngine {
  IBusEngine parent;
  GString *preedit;
  gint cursor_pos;
  IBusLookupTable *table;
  char *breakerList;
};

struct _IBusVarnamEngineClass {
  IBusEngineClass parent;
};

/* functions prototype */
/*static void	ibus_varnam_engine_class_init	(IBusVarnamEngineClass	*klass);*/
/*static void	ibus_varnam_engine_init		(IBusVarnamEngine		*engine);*/
static void	ibus_varnam_engine_destroy (IBusVarnamEngine *engine);
static gboolean ibus_varnam_engine_process_key_event (IBusEngine  *engine, guint keyval, guint keycode, guint modifiers);
static char* get_word_breakers();
/*[>static void ibus_varnam_engine_focus_in    (IBusEngine             *engine);<]*/
/*static void ibus_varnam_engine_focus_out   (IBusEngine             *engine);*/
/*static void ibus_varnam_engine_reset       (IBusEngine             *engine);*/
/*static void ibus_varnam_engine_disable     (IBusEngine             *engine);*/
/*static void ibus_engine_set_cursor_location (IBusEngine             *engine,*/
                                             /*gint                    x,*/
                                             /*gint                    y,*/
                                             /*gint                    w,*/
                                             /*gint                    h);*/
/*static void ibus_varnam_engine_set_capabilities*/
                                            /*(IBusEngine             *engine,*/
                                             /*guint                   caps);*/
/*static void ibus_varnam_engine_page_up     (IBusEngine             *engine);*/
/*static void ibus_varnam_engine_page_down   (IBusEngine             *engine);*/
/*static void ibus_varnam_engine_cursor_up   (IBusEngine             *engine);*/
/*static void ibus_varnam_engine_cursor_down (IBusEngine             *engine);*/
/*static void ibus_enchant_property_activate  (IBusEngine             *engine,*/
                                             /*const gchar            *prop_name,*/
                                             /*gint                    prop_state);*/
/*static void ibus_varnam_engine_property_show*/
											/*(IBusEngine             *engine,*/
                                             /*const gchar            *prop_name);*/
/*static void ibus_varnam_engine_property_hide*/
											/*(IBusEngine             *engine,*/
                                             /*const gchar            *prop_name);*/


G_DEFINE_TYPE (IBusVarnamEngine, ibus_varnam_engine, IBUS_TYPE_ENGINE)

static varnam *handle = NULL;
void varnam_engine_init_handle (const gchar *langCode)
{
  char *msg;
  int rc;

  if (handle == NULL) {
    rc = varnam_init_from_id (langCode, &handle, &msg);
    if (rc != VARNAM_SUCCESS) {
      g_message ("Error initializing varnam. %s\n", msg);
      handle = NULL;
    }
  }
}

static void
ibus_varnam_engine_class_init (IBusVarnamEngineClass *klass)
{
  IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (klass);
	IBusEngineClass *engine_class = IBUS_ENGINE_CLASS (klass);

  ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_varnam_engine_destroy;
  engine_class->process_key_event = ibus_varnam_engine_process_key_event;
}

static void
ibus_varnam_engine_init (IBusVarnamEngine *engine)
{
  IBusProperty *foo;
  IBusPropList *properties;

  engine->preedit = g_string_new ("");
  engine->cursor_pos = 0;
  engine->table = ibus_lookup_table_new (9, 0, TRUE, TRUE);
  engine->breakerList = get_word_breakers();
  ibus_lookup_table_set_orientation (engine->table, IBUS_ORIENTATION_VERTICAL);
  g_object_ref_sink (engine->table);

  foo = ibus_property_new ("foo", PROP_TYPE_TOGGLE, ibus_text_new_from_string ("Test foo"), NULL, NULL, false, true, PROP_STATE_UNCHECKED, NULL);
  properties = ibus_prop_list_new ();
  ibus_prop_list_append (properties, foo);
  ibus_engine_register_properties ((IBusEngine *) engine, properties);
}

static void
ibus_varnam_engine_destroy (IBusVarnamEngine *engine)
{
  if (engine->preedit) {
    g_string_free (engine->preedit, TRUE);
    engine->preedit = NULL;
  }

  if (engine->table) {
    g_object_unref (engine->table);
    engine->table = NULL;
  }

  ((IBusObjectClass *) ibus_varnam_engine_parent_class)->destroy ((IBusObject *) engine);
}

static void
ibus_varnam_engine_update_lookup_table (IBusVarnamEngine *engine)
{
  varray *words;
  vword *word;
  int rc, i;

  if (engine->preedit->len == 0) {
    ibus_engine_hide_lookup_table ((IBusEngine *) engine);
    return;
  }

  ibus_lookup_table_clear (engine->table);

  rc = varnam_transliterate (handle, engine->preedit->str, &words);
  if (rc != VARNAM_SUCCESS) {
    g_message ("Error transliterating: %s. %s\n", engine->preedit->str, varnam_get_last_error (handle));
    ibus_engine_hide_lookup_table ((IBusEngine *) engine);
    return;
  }

  for (i = 0; i < varray_length (words); i++) {
    word = varray_get (words, i);
    ibus_lookup_table_append_candidate (engine->table, ibus_text_new_from_string (word->text));
  }
  ibus_lookup_table_append_candidate (engine->table, ibus_text_new_from_string (engine->preedit->str));
  ibus_engine_update_lookup_table ((IBusEngine *) engine, engine->table, TRUE);
}

static void
ibus_varnam_engine_update_lookup_table_with_text(IBusVarnamEngine *engine, gchar *text){
  varray *words;
  vword *word;
  int rc, i;

  if (engine->preedit->len == 0) {
    ibus_engine_hide_lookup_table ((IBusEngine *) engine);
    return;
  }

  ibus_lookup_table_clear (engine->table);

  rc = varnam_transliterate (handle, text, &words);
  if (rc != VARNAM_SUCCESS) {
    g_message ("Error transliterating: %s. %s\n", text, varnam_get_last_error (handle));
    ibus_engine_hide_lookup_table ((IBusEngine *) engine);
    return;
  }

  for (i = 0; i < varray_length (words); i++) {
    word = varray_get (words, i);
    ibus_lookup_table_append_candidate (engine->table, ibus_text_new_from_string (word->text));
  }
  ibus_lookup_table_append_candidate (engine->table, ibus_text_new_from_string (engine->preedit->str));
  ibus_engine_update_lookup_table ((IBusEngine *) engine, engine->table, TRUE);
}

static void
ibus_varnam_engine_update_preedit (IBusVarnamEngine *engine)
{
  IBusText *text;

  text = ibus_text_new_from_static_string (engine->preedit->str);
  ibus_engine_update_preedit_text ((IBusEngine *) engine,
                                   text,
                                   engine->cursor_pos,
                                   TRUE);
}

static void
ibus_varnam_engine_clear_state (IBusVarnamEngine *engine)
{
  g_string_assign (engine->preedit, "");
  engine->cursor_pos = 0;
  ibus_varnam_engine_update_preedit (engine);
  ibus_lookup_table_clear (engine->table);
  ibus_engine_hide_lookup_table ((IBusEngine*) engine);
}

static gboolean
ibus_varnam_engine_commit (IBusVarnamEngine *engine, IBusText *text, gboolean shouldLearn)
{
  int rc;
  if (shouldLearn) {
    rc = varnam_learn (handle, ibus_text_get_text (text));
    if (rc != VARNAM_SUCCESS) {
      g_message ("Failed to learn: %s\n", ibus_text_get_text (text));
    }
  }

   /* Not releasing candidate because it is a borrowed reference */
  ibus_engine_commit_text ((IBusEngine *) engine, text);
  ibus_varnam_engine_clear_state (engine);
  return TRUE;
}

static IBusText*
ibus_varnam_engine_get_candidate_at (IBusVarnamEngine *engine, guint index)
{
  if (ibus_lookup_table_get_number_of_candidates (engine->table) == 0)
    return NULL;

  return ibus_lookup_table_get_candidate (engine->table, index);
}

static IBusText*
ibus_varnam_engine_get_candidate (IBusVarnamEngine *engine)
{
  return ibus_varnam_engine_get_candidate_at (engine, ibus_lookup_table_get_cursor_pos (engine->table));
}

static gboolean
ibus_varnam_engine_commit_candidate_at (IBusVarnamEngine *engine, guint index)
{
  IBusText *text;
  text = ibus_varnam_engine_get_candidate_at (engine, index);
  if (text != NULL) {
    return ibus_varnam_engine_commit (engine, text, TRUE);
  }

  if (ibus_lookup_table_get_number_of_candidates (engine->table) != 0) {
    /* Candidate window is visible and we are unable to get the candidate text at specified index.
     * This happens only when user specifies an index which is above the available candidates count.
     * In this case, returning TRUE so that this keystroke will not be considered */
    return TRUE;
  }

  return FALSE;
}

#define is_alpha(c) (((c) >= IBUS_a && (c) <= IBUS_z) || ((c) >= IBUS_A && (c) <= IBUS_Z))

static char*
get_word_breakers()
{ 
  static char *breakerList=0;
  int allocatedSize=1;

  if(breakerList == 0)
  {
    breakerList = (char*)malloc(allocatedSize);
    /*varnam_word_breakers calls realloc on breakerList if
    memory allocated is too small*/
    varnam_word_breakers(handle, breakerList, allocatedSize);
  }

  return breakerList;
}

static gboolean
is_word_breaker (guint keyval, char *breakerList)
{

  int i;

  for(i=0; breakerList[i] != '\0'; i++)
  {
    if((int)breakerList[i] == keyval)
      return true;
  }

  return false;
}

static gboolean
ibus_varnam_engine_process_key_event (IBusEngine *engine,
                                       guint       keyval,
                                       guint       keycode,
                                       guint       modifiers)
{
  IBusText *text, *tmp;
  IBusVarnamEngine *varnamEngine = (IBusVarnamEngine *) engine;
  int ncandidates = 0;

  if (modifiers & IBUS_RELEASE_MASK)
    return FALSE;

  modifiers &= (IBUS_CONTROL_MASK | IBUS_MOD1_MASK);

  if (modifiers != 0) {
    if (varnamEngine->preedit->len == 0)
      return FALSE;
    else
      return TRUE;
  }
 
  ncandidates = ibus_lookup_table_get_number_of_candidates(varnamEngine->table);

  switch (keyval) {
    case IBUS_space:
      text = ibus_varnam_engine_get_candidate (varnamEngine);
      if (text == NULL) {
        tmp = ibus_text_new_from_printf ("%s ", varnamEngine->preedit->str);
        return ibus_varnam_engine_commit (varnamEngine, tmp, FALSE);
      }
      tmp = ibus_text_new_from_printf ("%s ", ibus_text_get_text (text));
      return ibus_varnam_engine_commit (varnamEngine, tmp, TRUE);

    case IBUS_Return:
      text = ibus_varnam_engine_get_candidate (varnamEngine);
      if (text == NULL) {
        tmp = ibus_text_new_from_static_string (varnamEngine->preedit->str);
        ibus_varnam_engine_commit (varnamEngine, tmp, FALSE);
        return FALSE;
      }
      return ibus_varnam_engine_commit (varnamEngine, text, TRUE);

    case IBUS_Escape:
      if (varnamEngine->preedit->len == 0)
        return FALSE;

      tmp = ibus_text_new_from_static_string (varnamEngine->preedit->str);
      ibus_varnam_engine_commit (varnamEngine, tmp, FALSE);
      return FALSE;

    case IBUS_Left:
      if (varnamEngine->preedit->len == 0)
        return FALSE;
      if (varnamEngine->cursor_pos > 0) {
        varnamEngine->cursor_pos --;
        ibus_varnam_engine_update_preedit (varnamEngine);
      }
      return TRUE;

    case IBUS_Right:
      if (varnamEngine->preedit->len == 0)
        return FALSE;
      if (varnamEngine->cursor_pos < varnamEngine->preedit->len) {
        varnamEngine->cursor_pos ++;
        ibus_varnam_engine_update_preedit (varnamEngine);
      }
      return TRUE;

    case IBUS_Up:
      if (varnamEngine->preedit->len == 0)
        return FALSE;

      ibus_lookup_table_cursor_up (varnamEngine->table);
      ibus_engine_update_lookup_table (engine, varnamEngine->table, TRUE);
      return TRUE;

    case IBUS_Down:
      if (varnamEngine->preedit->len == 0)
        return FALSE;

      ibus_lookup_table_cursor_down (varnamEngine->table);
      ibus_engine_update_lookup_table (engine, varnamEngine->table, TRUE);

      return TRUE;

    case IBUS_BackSpace:
      if (varnamEngine->preedit->len == 0)
        return FALSE;
      if (varnamEngine->cursor_pos > 0) {
        varnamEngine->cursor_pos --;
        g_string_erase (varnamEngine->preedit, varnamEngine->cursor_pos, 1);
        ibus_varnam_engine_update_preedit (varnamEngine);
        ibus_varnam_engine_update_lookup_table (varnamEngine);
        text = ibus_varnam_engine_get_candidate (varnamEngine);
        ibus_engine_hide_preedit_text((IBusEngine*)varnamEngine);
        ibus_engine_update_preedit_text((IBusEngine*)varnamEngine, text, varnamEngine->cursor_pos, TRUE);
        if (varnamEngine->preedit->len == 0) {
          /* Current backspace has cleared the preedit. Need to reset the engine state */
          ibus_varnam_engine_clear_state (varnamEngine);
        }
      }
      return TRUE;

    case IBUS_Delete:
      if (varnamEngine->preedit->len == 0)
        return FALSE;
      if (varnamEngine->cursor_pos < varnamEngine->preedit->len) {
        g_string_erase (varnamEngine->preedit, varnamEngine->cursor_pos, 1);
        ibus_varnam_engine_update_preedit (varnamEngine);
        ibus_varnam_engine_update_lookup_table (varnamEngine);
      }
      return TRUE;
    
    case IBUS_1:
      return ibus_varnam_engine_commit_candidate_at (varnamEngine, 0);
    case IBUS_2:
      return ibus_varnam_engine_commit_candidate_at (varnamEngine, 1);
    case IBUS_3:
      return ibus_varnam_engine_commit_candidate_at (varnamEngine, 2);
    case IBUS_4:
      return ibus_varnam_engine_commit_candidate_at (varnamEngine, 3);
    case IBUS_5:
      return ibus_varnam_engine_commit_candidate_at (varnamEngine, 4);
    case IBUS_6:
      return ibus_varnam_engine_commit_candidate_at (varnamEngine, 5);
    case IBUS_7:
      return ibus_varnam_engine_commit_candidate_at (varnamEngine, 6);
    case IBUS_8:
      return ibus_varnam_engine_commit_candidate_at (varnamEngine, 7);
    case IBUS_9:
      return ibus_varnam_engine_commit_candidate_at (varnamEngine, 8);
    case IBUS_KP_1:
      return ibus_varnam_engine_commit_candidate_at (varnamEngine, 0);
    case IBUS_KP_2:
      return ibus_varnam_engine_commit_candidate_at (varnamEngine, 1);
    case IBUS_KP_3:
      return ibus_varnam_engine_commit_candidate_at (varnamEngine, 2);
    case IBUS_KP_4:
      return ibus_varnam_engine_commit_candidate_at (varnamEngine, 3);
    case IBUS_KP_5:
      return ibus_varnam_engine_commit_candidate_at (varnamEngine, 4);
    case IBUS_KP_6:
      return ibus_varnam_engine_commit_candidate_at (varnamEngine, 5);
    case IBUS_KP_7:
      return ibus_varnam_engine_commit_candidate_at (varnamEngine, 6);
    case IBUS_KP_8:
      return ibus_varnam_engine_commit_candidate_at (varnamEngine, 7);
    case IBUS_KP_9:
      return ibus_varnam_engine_commit_candidate_at (varnamEngine, 8);

  }

  if (is_word_breaker (keyval, varnamEngine->breakerList)) {
    text = ibus_varnam_engine_get_candidate (varnamEngine);
    if (text != NULL) {
      tmp = ibus_text_new_from_printf ("%s%c", ibus_text_get_text (text), keyval);
      return ibus_varnam_engine_commit (varnamEngine, tmp, TRUE);
    }

    return FALSE;
  }

  if (keyval <= 128) {
    if (varnamEngine->preedit->len == 0) {
      /* We are starting a new word. Now there could be a word selected in the text field
       * and we may be typing over the selection. In this case to clear the selection
       * we commit a empty text which will trigger the textfield to clear the selection.
       * If there is no selection, this won't affect anything */
      tmp = ibus_text_new_from_static_string ("");
      ibus_engine_commit_text ((IBusEngine *) varnamEngine, tmp);
    }
    g_string_insert_c (varnamEngine->preedit, varnamEngine->cursor_pos, keyval);
    varnamEngine->cursor_pos ++;
    /*ibus_varnam_engine_update_preedit (varnamEngine);*/
    /*ibus_varnam_engine_update_lookup_table (varnamEngine);*/
    ibus_varnam_engine_update_lookup_table_with_text(varnamEngine, varnamEngine->preedit->str);
    text = ibus_varnam_engine_get_candidate (varnamEngine);
 	  ibus_engine_hide_preedit_text((IBusEngine*)varnamEngine);
  	ibus_engine_update_preedit_text((IBusEngine*)varnamEngine, text, varnamEngine->cursor_pos, TRUE);
    return TRUE;
  }

  return FALSE;
}
