#ifndef ML_XML_H
#define ML_XML_H

#include "minilang.h"

#ifdef __cplusplus
extern "C" {
#endif

void ml_xml_init(stringmap_t *Globals);

extern ml_type_t MLXmlT[];
extern ml_type_t MLXmlTextT[];
extern ml_type_t MLXmlElementT[];

typedef struct ml_xml_node_t ml_xml_node_t;
typedef struct ml_xml_element_t ml_xml_element_t;

ml_xml_element_t *ml_xml_node_parent(ml_xml_node_t *Value);
ml_xml_node_t *ml_xml_node_next(ml_xml_node_t *Value);
ml_xml_node_t *ml_xml_node_prev(ml_xml_node_t *Value);

ml_value_t *ml_xml_element_tag(ml_xml_element_t *Value);
ml_value_t *ml_xml_element_attributes(ml_xml_element_t *Value);
size_t ml_xml_element_length(ml_xml_element_t *Value);
ml_xml_node_t *ml_xml_element_head(ml_xml_element_t *Value);

ml_xml_element_t *ml_xml_element(const char *Tag);
ml_xml_node_t *ml_xml_text(const char *Content, int Length);
void ml_xml_element_put(ml_xml_element_t *Parent, ml_xml_node_t *Child);

ml_value_t *ml_parser_escape_xml_like(ml_parser_t *Parser, ml_value_t *Constructor);

#ifdef __cplusplus
}
#endif

#endif
