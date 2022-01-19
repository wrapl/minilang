#ifndef ML_XML_H
#define ML_XML_H

#include "minilang.h"

void ml_xml_init(stringmap_t *Globals);

extern ml_type_t MLXmlT[];
extern ml_type_t MLXmlTextT[];
extern ml_type_t MLXmlElementT[];

ml_value_t *ml_xml_node_next(ml_value_t *Value);
ml_value_t *ml_xml_node_prev(ml_value_t *Value);

ml_value_t *ml_xml_element_tag(ml_value_t *Value);
ml_value_t *ml_xml_element_attributes(ml_value_t *Value);
size_t ml_xml_element_length(ml_value_t *Value);
ml_value_t *ml_xml_element_head(ml_value_t *Value);

#endif
