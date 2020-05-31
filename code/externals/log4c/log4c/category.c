static const char version[] = "$Id: category.c,v 1.13 2013/09/29 17:37:04 valtri Exp $";

/*
* category.c
*
* Copyright 2001-2003, Meiosys (www.meiosys.com). All rights reserved.
*
* See the COPYING file for the terms of usage and distribution.
*/
#include "config.h"

#include <stdlib.h>
#include <string.h>
#if defined(HAVE_ALLOCA_H)
#include <alloca.h>
#elif defined(HAVE_MALLOC_H)
#include <malloc.h>
#endif
#include <sd/sprintf.h>
#include <sd/malloc.h>
#include <sd/factory.h>
#include <log4c/appender.h>
#include <log4c/priority.h>
#include <log4c/logging_event.h>
#include <log4c/category.h>
#include <log4c/rc.h>
#include <sd/error.h>
#include <sd/sd_xplatform.h>

struct __log4c_category {
  char*			cat_name;
  int				cat_priority;
  int				cat_additive;
  const log4c_category_t*	cat_parent;
  log4c_appender_t*		cat_appender;
};

sd_factory_t* log4c_category_factory = NULL;

static const char LOG4C_CATEGORY_DEFAULT[] = "root";


/**
* @bug the root category name should be "" not "root". *
*/

/*******************************************************************************/
extern log4c_category_t* log4c_category_get(const char* a_name)
{
  static const sd_factory_ops_t log4c_category_factory_ops = {
    (void*) log4c_category_new,
    (void*) log4c_category_delete,
    (void*) log4c_category_print,
  };
  
  if (!log4c_category_factory) {
    log4c_category_factory = sd_factory_new("log4c_category_factory",
      &log4c_category_factory_ops);
  }
  
  return sd_factory_get(log4c_category_factory, a_name);
}

/*******************************************************************************/
extern int log4c_category_list(log4c_category_t** a_cats, int a_ncats)
{
  return sd_factory_list(log4c_category_factory, (void**) a_cats, a_ncats);
}

/*******************************************************************************/
static const char* dot_dirname(char* a_string);

extern log4c_category_t* log4c_category_new(const char* a_name)
{
  log4c_category_t* thisp;
  
  if (!a_name)
    return NULL;
  
  thisp		= sd_calloc(1, sizeof(log4c_category_t));
  thisp->cat_name	= je_strdup2(a_name);
  thisp->cat_priority	= LOG4C_PRIORITY_NOTSET;
  thisp->cat_additive	= 1;
  thisp->cat_appender	= NULL;
  thisp->cat_parent	= NULL;
  
  /* skip root category because it has a NULL parent */
  if (strcmp(LOG4C_CATEGORY_DEFAULT, a_name)) {
    char* tmp = je_strdup2(thisp->cat_name);
    
    thisp->cat_parent = log4c_category_get(dot_dirname(tmp));
    free(tmp);
  }
  return thisp;
}

/*******************************************************************************/
extern void log4c_category_delete(log4c_category_t* thisp)
{
  if (!thisp) 
    return;
  
  free(thisp->cat_name);
  free(thisp);
}

/*******************************************************************************/
extern const char* log4c_category_get_name(const log4c_category_t* thisp)
{
  return (thisp ? thisp->cat_name : "(nil)");
}

/*******************************************************************************/
extern int log4c_category_get_priority(const log4c_category_t* thisp)
{
  return (thisp ? thisp->cat_priority : LOG4C_PRIORITY_UNKNOWN);
}

/*******************************************************************************/
extern int log4c_category_get_chainedpriority(const log4c_category_t* thisp)
{
  const log4c_category_t* cat = thisp;
  
  if (!thisp) 
    return LOG4C_PRIORITY_UNKNOWN;
  
  while (cat->cat_priority == LOG4C_PRIORITY_NOTSET && cat->cat_parent)
    cat = cat->cat_parent;
	
  return cat->cat_priority;
}

/*******************************************************************************/
extern const log4c_appender_t* log4c_category_get_appender(const log4c_category_t* thisp)
{
  return (thisp ? thisp->cat_appender : NULL);
}

/*******************************************************************************/
extern int log4c_category_get_additivity(const log4c_category_t* thisp)
{
  return (thisp ? thisp->cat_additive : -1);
}

/*******************************************************************************/
extern int log4c_category_set_priority(log4c_category_t* thisp, int a_priority)
{
  int previous;
  
  if (!thisp) 
    return LOG4C_PRIORITY_UNKNOWN;
  
  previous = thisp->cat_priority;
  thisp->cat_priority = a_priority;
  return previous;
}

/**
* @todo need multiple appenders per category
*/

/*******************************************************************************/
extern const log4c_appender_t* log4c_category_set_appender(
  log4c_category_t* thisp, 
  log4c_appender_t* a_appender)
{
  log4c_appender_t* previous;
  
  if (!thisp) 
    return NULL;
  
  previous = thisp->cat_appender;
  thisp->cat_appender = a_appender;
  return previous;
}

/*******************************************************************************/
extern int log4c_category_set_additivity(log4c_category_t* thisp, int a_additivity)
{
  int previous;
  
  if (!thisp) 
    return -1;
  
  previous = thisp->cat_additive;
  thisp->cat_additive = a_additivity;
  return previous;
}

/*******************************************************************************/
extern void log4c_category_print(const log4c_category_t* thisp, FILE* a_stream)
{
  if (!thisp) 
    return;
  
    fprintf(a_stream, "{ name:'%s' priority:%s additive:%d appender:'%s' parent:'%s' }",
	    thisp->cat_name,
	    log4c_priority_to_string(thisp->cat_priority),
	    thisp->cat_additive,
	    log4c_appender_get_name(thisp->cat_appender),
	    log4c_category_get_name(thisp->cat_parent)
      );
}

/*******************************************************************************/
extern void __log4c_category_vlog(const log4c_category_t* thisp, 
  const log4c_location_info_t* a_locinfo, 
  int a_priority,
  const char* a_format, 
  va_list a_args)
{
  char* message;
  log4c_logging_event_t evt;
  const log4c_category_t* cat;
  int yes = 0;
  
  if (!thisp)
    return;
  
  /* check if an appender is defined in the category hierarchy */
  for (cat = thisp; cat; cat = cat->cat_parent) {
    if (cat->cat_appender)
	    yes++;
  }
  
  if (!yes)
    return;

  log4c_reread();

  /* when there is no limit on the buffer size, we use malloc() to
  * give the user the possiblity to reallocate if necessary. When
  * the buffer is limited in size, we use alloca() for more
  * efficiency.
  */
  evt.evt_buffer.buf_maxsize = log4c_rc->config.bufsize;
  
  if (!evt.evt_buffer.buf_maxsize) {
    evt.evt_buffer.buf_size = LOG4C_BUFFER_SIZE_DEFAULT;
    evt.evt_buffer.buf_data = sd_malloc(evt.evt_buffer.buf_size);
    message = sd_vsprintf(a_format, a_args);
  }
  else {
    size_t n;
    
    evt.evt_buffer.buf_size = evt.evt_buffer.buf_maxsize;
    evt.evt_buffer.buf_data = alloca(evt.evt_buffer.buf_size);
    message = alloca(evt.evt_buffer.buf_size);
    
    if ( (n = (size_t)vsnprintf(message, evt.evt_buffer.buf_size, a_format, a_args))
      >= evt.evt_buffer.buf_size)
    sd_error("truncating message of %d bytes (bufsize = %d)", n, 
      evt.evt_buffer.buf_size);
  }
  
  evt.evt_category	= thisp->cat_name;
  evt.evt_priority	= a_priority;
  evt.evt_msg	        = message;
  evt.evt_loc	        = a_locinfo;
  SD_GETTIMEOFDAY(&evt.evt_timestamp, NULL);
  
  for (cat = thisp; cat; cat = cat->cat_parent) {
    if (cat->cat_appender)
	    log4c_appender_append(cat->cat_appender, &evt);
    
    if (!cat->cat_additive) break;	
  }
  
  if (!evt.evt_buffer.buf_maxsize) {
    free(message);
    free(evt.evt_buffer.buf_data);
  }
}

/*******************************************************************************/
static const char* dot_dirname(char* a_string)
{
  char* p;
  
  if (!a_string)
    return NULL;
  
  if ( (p = strrchr(a_string, '.')) == NULL)
    return LOG4C_CATEGORY_DEFAULT;
  
  *p = '\0';
  return a_string;
}

