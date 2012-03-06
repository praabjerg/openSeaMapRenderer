/* Copyright 2012 Malcolm Herring
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * For a copy of the GNU General Public License, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "seamaps.h"

void lightstring(int, char*);

char line[1000];
char *tokens;
char *ele;
char *key;
char *prefix;
char *attribute;
char *value;
char *item;
char str[250];
char colstr[40];
boolean node = FALSE;
boolean way = FALSE;
int sector;
int waynodes;

int secmax;
struct GROUP {
  int matches;
  int matching;
  Col_t colours;
  char charstr[30];
} group[4];

typedef struct {
  Col_t col;
  Col_t alt;
  char chr[20];
  char grp[20];
  int per;
  int hgt;
  int rng;
  Lit_t lit;
  int mlt;
  float rad;
} Lgt_t;

struct seamark_s {
  boolean isLit;
  Obj_t obj;
  Shp_t shp;
  Cat_t cat;
  Fog_t fog;
  char fog_grp[40];
  char fog_per[40];
  char fog_rng[40];
  Rtb_t rtb;
  char rtb_grp[40];
  char rtb_per[40];
  char rtb_rng[40];
  Lgt_t lgt[30];
} seamark;

int main (int argc, const char * argv[]) {
  
	while (fgets(line, 1000, stdin) != NULL) {
    tokens = strdup(line);
		ele = strtok(tokens, " <");
    
    /* Start new seamark node */
    
    if ((strcmp(ele, "node") == 0) && (strstr(line, "/>") == NULL)) {
      node = TRUE;
      secmax = 0;
      bzero(&seamark, sizeof(struct seamark_s));
      bzero(&group[0], 4*sizeof(struct GROUP));
    }
    
    if (strcmp(ele, "way") == 0) {
      way = TRUE;
      sector = 0;
      waynodes = 0;
    }
    
    if (strcmp(ele, "nd") == 0) {
      waynodes++;
    }
    
    /* Complete seamark node */
    
    if (strcmp(ele, "/node>\n") == 0) {
      if (seamark.shp != UNKSHP) {
        
        /* Resolve perch shape */
        
        if (seamark.shp == PERCH) {
          switch (seamark.cat) {
            case LAM_PORT:
              printf("<tag k=\"seamark:body_shape\" v=\"perch_port\"/>\n");
              break;
            case LAM_STBD:
              printf("<tag k=\"seamark:body_shape\" v=\"perch_starboard\"/>\n");
              break;
            default:
              printf("<tag k=\"seamark:body_shape\" v=\"stake\"/>\n");
              break;
          }
        } else {
          printf("<tag k=\"seamark:body_shape\" v=\"%s\"/>\n", (char*)getval((Map_t*)ShpSTR, (Key_t)seamark.shp));
        }
      }
      
      /* Construct fog signal string */
      
      if (seamark.fog != NOFOG) {
        strcpy(str, getval((Map_t*)FogMAP, (Key_t)seamark.fog));
        if (strlen(seamark.fog_grp) > 0)
          sprintf(strchr(str, 0), "(%s)", seamark.fog_grp);
        else if ((strlen(seamark.fog_per) > 0) || (strlen(seamark.fog_rng) > 0))
          strcat(str, " ");
        if (strlen(seamark.fog_per) > 0)
          sprintf(strchr(str, 0), "%ss ", seamark.fog_per);
        if (strlen(seamark.fog_rng) > 0)
          sprintf(strchr(str, 0), "%sM", seamark.fog_rng);
        printf("<tag k=\"seamark:fog_char\" v=\"%s\"/>\n", str);
      }
      
      /* Construct radar transponder string */
      
      if (seamark.rtb != NORTB) {
        strcpy(str, getval((Map_t*)RtbMAP, (Key_t)seamark.rtb));
        if (strlen(seamark.rtb_grp) > 0)
          sprintf(strchr(str, 0), "(%s)", seamark.rtb_grp);
        else if ((strlen(seamark.rtb_per) > 0) || (strlen(seamark.rtb_rng) > 0))
          strcat(str, " ");
        if (strlen(seamark.rtb_per) > 0)
          sprintf(strchr(str, 0), "%ss ", seamark.rtb_per);
        if (strlen(seamark.rtb_rng) > 0)
          sprintf(strchr(str, 0), "%sM", seamark.rtb_rng);
        printf("<tag k=\"seamark:rtb_char\" v=\"%s\"/>\n", str);
      }
      if (seamark.isLit) {
        
        /* Sort sectors into groups with same characteristics */
        
        if (secmax > 0) {
          int i, j, k;
          for (i = 1; i <= secmax; i++) {
            if (strlen(seamark.lgt[i].chr) > 0)
              strcpy(str, seamark.lgt[i].chr);
            else if (strlen(seamark.lgt[0].chr) > 0)
              strcpy(str, seamark.lgt[0].chr);
            else strcpy(str, "");
            if (strncmp(str, "Al.", 3) == 0)
              strcpy(str, &str[3]);
            if (strcmp(str, "Al") == 0)
              strcpy(str, "F");
            if ((seamark.lgt[i].lit == DIR) || (seamark.lgt[0].lit == DIR))
              strcat(str, "Dir");
            if (seamark.lgt[i].per != 0)
              sprintf(strchr(str, 0), "%ds ", seamark.lgt[i].per);
            else if (seamark.lgt[0].per != 0)
              sprintf(strchr(str, 0), "%ds ", seamark.lgt[0].per);
            for (j = 0; j < 4; j++) {
              if (*group[j].charstr == 0) {
                strcpy(group[j].charstr, str);
                group[j].matches = 1;
                group[j].matching = (1 << i);
                break;
              } else {
                if (strcmp(group[j].charstr, str) == 0) {
                  group[j].matches++;
                  group[j].matching |= (1 << i);
                  break;
                }
              }
            }
          }
          
          /* Construct light strings for two most common groups */
          
          for (i = j = k = 0; i < 4; i++) {
            if (group[i].matches > j)
              j = group[i].matches;
            if ((group[i].matches > k) && (group[i].matches < j))
              k = group[i].matches;
          }
          if (j > 0) {
            for (i = 0; i < 4; i++) {
              if (group[i].matches == j) {
                lightstring(i, "seamark:light_primary");
                break;
              }
            }
            if (k > 0) {
              for (i = 0; i < 4; i++) {
                if (group[i].matches == k) {
                  lightstring(i, "seamark:light_secondary");
                  break;
                }
              }
            }
          }
        } else {
          
          /* Construct light strings for non-sectored lights */
          
          strcpy(str, "");
          strcpy(colstr, "");
          if (seamark.lgt[0].lit == DIR)
            strcat(str, "Dir");
          if (seamark.lgt[0].mlt > 0)
            sprintf(strchr(str, 0), "%d", seamark.lgt[0].mlt);
          if (strlen(seamark.lgt[0].chr) > 0)
            sprintf(strchr(str, 0), "%s", seamark.lgt[0].chr);
          if (strlen(seamark.lgt[0].grp) > 0)
            sprintf(strchr(str, 0), "(%s)", seamark.lgt[0].grp);
          if (seamark.lgt[0].col != UNKCOL) {
            if ((seamark.lgt[0].col != WHITE) || (seamark.lgt[0].alt != UNKCOL)) {
              if ((strlen(str) > 0) && (strlen(seamark.lgt[0].grp) == 0))
                strcat(str, ".");
              strcat(str, getval((Map_t*)ColMAP, (Key_t)seamark.lgt[0].col));
            } 
            strcat(colstr, getval((Map_t*)ColMAP, (Key_t)seamark.lgt[0].col));
            if (seamark.lgt[0].alt != UNKCOL) {
              strcat(str, getval((Map_t*)ColMAP, (Key_t)seamark.lgt[0].alt));
              strcat(colstr, getval((Map_t*)ColMAP, (Key_t)seamark.lgt[0].alt));
            }
          }
          if ((seamark.lgt[0].lit != UNKLIT) && (seamark.lgt[0].lit < DIR))
            sprintf(strchr(str, 0), "(%s)", getval((Map_t*)LitMAP, (Key_t)seamark.lgt[0].lit));
          if (seamark.lgt[0].per > 0)
            sprintf(strchr(str, 0), " %ds", seamark.lgt[0].per);
          if (seamark.lgt[0].hgt > 0)
            sprintf(strchr(str, 0), " %dm", seamark.lgt[0].hgt);
          if (seamark.lgt[0].rng > 0)
            sprintf(strchr(str, 0), " %dM", seamark.lgt[0].rng);
          printf("<tag k=\"seamark:light_primary\" v=\"%s\"/>\n", str);
          if (strlen(colstr) > 0)
            printf("<tag k=\"seamark:light_colour\" v=\"%s\"/>\n", colstr);
        }
      }
      node = FALSE;
    }
    
    /* Complete seamark way */
    
    if  (strcmp(ele, "/way>\n") == 0) {
      
      /* Construct light strings for sector arcs */
      
      if ((sector != 0) && (seamark.lgt[sector].lit != DIR) && (seamark.lgt[0].lit != DIR) && (seamark.lgt[sector].rad > 0.2)) {
        if (seamark.lgt[sector].alt != UNKCOL)
          strcpy(str, "Al.");
        else
          strcpy(str, "");
        if (strlen(seamark.lgt[sector].chr) > 0)
          sprintf(strchr(str, 0), "%s", seamark.lgt[sector].chr);
        else if (strlen(seamark.lgt[0].chr) > 0)
          sprintf(strchr(str, 0), "%s", seamark.lgt[0].chr);
        if (strlen(seamark.lgt[sector].grp) > 0)
          sprintf(strchr(str, 0), "(%s)", seamark.lgt[sector].grp);
        else if (strlen(seamark.lgt[0].grp) > 0)
          sprintf(strchr(str, 0), "(%s)", seamark.lgt[0].grp);
        else if (strlen(str) > 0)
          strcat(str, ".");
        strcat(str, getval((Map_t*)ColMAP, (Key_t)seamark.lgt[sector].col));
        if (seamark.lgt[sector].alt != UNKCOL)
          strcat(str, getval((Map_t*)ColMAP, (Key_t)seamark.lgt[sector].alt));
        if (seamark.lgt[sector].per > 0)
          sprintf(strchr(str, 0), " %ds", seamark.lgt[sector].per);
        else if (seamark.lgt[0].per > 0)
          sprintf(strchr(str, 0), " %ds", seamark.lgt[0].per);
        printf("<tag k=\"seamark:arc_caption\" v=\"%s\"/>\n", str);
        printf("<tag k=\"seamark:arc_colour\" v=\"%s%s\"/>\n",
               getval((Map_t*)ColMAP, (Key_t)seamark.lgt[sector].col),
               getval((Map_t*)ColMAP, (Key_t)seamark.lgt[sector].alt));
        printf("<tag k=\"seamark:arc_nodes\" v=\"%d\"/>\n", waynodes);
      }
      way = FALSE;
    }
    
    /* Process tag */
    
    if (strcmp(ele, "tag") == 0) {
      strtok(NULL, "\"'");
      
      /* Extract key and value */
      
      key = strtok(NULL, "\"'");
      strtok(NULL, "\"'");
      if (strncmp(key, "seamark:", 8) != 0) continue;
      value = strtok(NULL, "\"'");
      
      /* Process key */
      
      prefix = strtok(key, ":");
      
      /* Process seamark tag */
      
      if (strcmp(prefix, "seamark") == 0) {
        prefix = strtok(NULL, ":");
        
        /* Process seamark:type tag */
        
        if (strcmp(prefix, "type") == 0) {
          seamark.obj = (Obj_t)getkey((Map_t*)ObjSTR, value);
          switch (seamark.obj) {
            case BCNCAR:
            case BCNISD:
              if (seamark.shp == UNKSHP)
                seamark.shp = BEACON;
            case BOYISD:
            case BOYCAR:
              printf("<tag k=\"seamark:class\" v=\"cardinal\"/>\n");
              break;
            case BCNLAT:
              if (seamark.shp == UNKSHP)
                seamark.shp = BEACON;
            case BOYLAT:
              printf("<tag k=\"seamark:class\" v=\"lateral\"/>\n");
              break;
            case BCNSAW:
              if (seamark.shp == UNKSHP)
                seamark.shp = BEACON;
            case BOYSAW:
              printf("<tag k=\"seamark:class\" v=\"fairway\"/>\n");
              break;
            case BCNSPP:
              if (seamark.shp == UNKSHP)
                seamark.shp = BEACON;
            case BOYSPP:
              printf("<tag k=\"seamark:class\" v=\"special\"/>\n");
              break;
            case MORFAC:
              break;
            case LNDMRK:
              if (seamark.shp == UNKSHP) {
                seamark.shp = HOUSE;
                printf("<tag k=\"seamark:class\" v=\"light\"/>\n");
              }
             break;
            case LITMAJ:
              seamark.shp = MAJOR;
              printf("<tag k=\"seamark:class\" v=\"light\"/>\n");
              break;
            case LITMIN:
              seamark.shp = MINOR;
              printf("<tag k=\"seamark:class\" v=\"light\"/>\n");
              break;
            case LITFLT:
              if (seamark.shp == UNKSHP)
                seamark.shp = FLOAT;
              printf("<tag k=\"seamark:class\" v=\"light\"/>\n");
              break;
            case LITVES:
              if (seamark.shp == UNKSHP)
                seamark.shp = SUPER;
              printf("<tag k=\"seamark:class\" v=\"light\"/>\n");
              break;
            case SISTAW:
            case SISTAT:
              seamark.shp = SIGNAL;
              break;
            case CGUSTA:
              seamark.shp = SIGNAL;
              printf("<tag k=\"seamark:sis_type\" v=\"CG\"/>\n");
              break;
            case OFSPLF:
              if (seamark.shp == UNKSHP)
                seamark.shp = PLATFORM;
              break;
            default:
              printf("%s", line);
          }
        } else {
          
          /* Process other seamark: tags */
          
          attribute = strtok(NULL, ":");
          switch (getkey((Map_t*)ObjSTR, prefix)) {
            case LIGHTS:
              seamark.isLit = TRUE;
              int idx = 0;
              if (isdigit(attribute[0])) {
                idx = atoi(attribute);
                secmax = idx > secmax ? idx : secmax;
                attribute = strtok(NULL, ":");
              }
              switch (getkey((Map_t*)AtlSTR, attribute)) {
                case COL:
                  item = strtok(value, ";");
                  seamark.lgt[idx].col = (Col_t)getkey((Map_t*)ColSTR, item);
                  item = strtok(NULL, ";");
                  if (item != NULL)
                    seamark.lgt[idx].alt = (Col_t)getkey((Map_t*)ColSTR, item);
                  break;
                case CHR:
                  strcpy(seamark.lgt[idx].chr, value);
                  break;
                case GRP:
                  strcpy(seamark.lgt[idx].grp, value);
                  break;
                case PER:
                  seamark.lgt[idx].per = atoi(value);
                  break;
                case LIT:
                  seamark.lgt[idx].lit = (Lit_t)getkey((Map_t*)LitSTR, value);
                  break;
                case HGT:
                  seamark.lgt[idx].hgt = atoi(value);
                  break;
                case RNG:
                  seamark.lgt[idx].rng = atoi(value);
                  break;
                case MLT:
                  seamark.lgt[idx].mlt = atoi(value);
                  break;
                case RAD:
                  item = strtok(value, ",;");
                  seamark.lgt[idx].rad = atof(item);
                  break;
                default:
                  break;
              }
              if (way && (strcmp(attribute, "sector_nr") == 0)) {
                sector = atoi(value);
              }
              break;
              
              /* Convert topmark attributes */
              
            case TOPMAR:
            case DAYMAR:
              switch (getkey((Map_t*)AttSTR, attribute)) {
                case SHAPE:
                  printf("<tag k=\"seamark:top_shape\" v=\"%s\"/>\n", value);
                  break;
                case COLOUR:
                  item = strtok(value, ";");
                  bzero(str, sizeof(str));
                  do {
                    Col_t col = (Col_t)getkey((Map_t*)ColSTR, item);
                    strcat(str, getval((Map_t*)ColMAP, (Key_t)col));
                    item = strtok(NULL, ";");
                  } while (item != NULL);
                  printf("<tag k=\"seamark:top_colour\" v=\"%s\"/>\n", str);
                  break;
                case COLPAT:
                  printf("<tag k=\"seamark:top_colour_pattern\" v=\"%s\"/>\n", value);
                  break;
                default:
                  printf("%s", line);
              }
              break;
              
              /* Save fog signal attributes */
              
            case FOGSIG:
              if (attribute == NULL)
                seamark.fog = UNKFOG;
              else
                switch (getkey((Map_t*)AttSTR, attribute)) {
                  case CATEGORY:
                    seamark.fog = (Fog_t)getkey((Map_t*)FogSTR, value);
                    break;
                  case GROUP:
                    strcpy(seamark.fog_grp, value);
                    break;
                  case PERIOD:
                    strcpy(seamark.fog_per, value);
                    break;
                  case RANGE:
                    strcpy(seamark.fog_rng, value);
                    break;
                  default:
                    printf("%s", line);
                }
              break;
              
              /* Save radar transponder attributes */
              
            case RTPBCN:
              switch (getkey((Map_t*)AttSTR, attribute)) {
                case CATEGORY:
                  seamark.rtb = (Rtb_t)getkey((Map_t*)RtbSTR, value);
                  break;
                case GROUP:
                  strcpy(seamark.rtb_grp, value);
                  break;
                case PERIOD:
                  strcpy(seamark.rtb_per, value);
                  break;
                case RANGE:
                  strcpy(seamark.rtb_rng, value);
                  break;
                default:
                  printf("%s", line);
              }
              break;
              
              /* Save signal station attributes */
              
            case SISTAT:
            case SISTAW:
              switch (getkey((Map_t*)AttSTR, attribute)) {
                case CATEGORY:
                  printf("<tag k=\"seamark:sis_type\" v=\"SS%s\"/>\n", getval((Map_t*)SisMAP, getkey((Map_t*)CatSTR, value)));
                  break;
                case CHANNEL:
                  printf("<tag k=\"seamark:sis_chan\" v=\"Ch%s\"/>\n", value);
                  break;
                default:
                  printf("%s", line);
              }
              break;
              
              /* Save mooring attributes */
              
            case MORFAC:
              switch (getkey((Map_t*)AttSTR, attribute)) {
                case CATEGORY:
                  seamark.cat = (Cat_t)getkey((Map_t*)CatSTR, value);
                  switch (seamark.cat) {
                    case MOR_BUOY:
                      if (seamark.shp == UNKSHP)
                        seamark.shp = SPHERI;
                      printf("<tag k=\"seamark:top_shape\" v=\"mooring\"/>\n");
                      break;
                    case MOR_DLPN:
                      seamark.shp = DOLPHIN;
                      break;
                    case MOR_DDPN:
                      seamark.shp = DEVDOLPHIN;
                      break;
                    case MOR_POST:
                      seamark.shp = POST;
                      break;
                    default:
                      break;
                  }
                  break;
                case SHAPE:
                  seamark.shp = (Shp_t)getkey((Map_t*)ShpSTR, value);
                  if (seamark.shp == STAKE)
                    seamark.shp = POST;
                  break;
                case COLOUR:
                  item = strtok(value, ";");
                  bzero(str, sizeof(str));
                  do {
                    Col_t col = (Col_t)getkey((Map_t*)ColSTR, item);
                    strcat(str, getval((Map_t*)ColMAP, (Key_t)col));
                    item = strtok(NULL, ";");
                  } while (item != NULL);
                  printf("<tag k=\"seamark:body_colour\" v=\"%s\"/>\n", str);
                  break;
                case COLPAT:
                  printf("<tag k=\"seamark:body_colour_pattern\" v=\"%s\"/>\n", value);
                  break;
                default:
                  printf("%s", line);
              }
              break;
              
              /* Process buoy & beacon attributes */
              
            case BCNCAR:
            case BCNISD:
            case BCNLAT:
            case BCNSAW:
            case BCNSPP:
            case BOYCAR:
            case BOYISD:
            case BOYLAT:
            case BOYSAW:
            case BOYSPP:
            case LITFLT:
            case LITVES:
              
              /* Save default buoy & beacon shapes */
              
              switch (getkey((Map_t*)ObjSTR, attribute)) {
                case BCNCAR:
                case BCNISD:
                case BCNLAT:
                case BCNSAW:
                case BCNSPP:
                  if (seamark.shp == UNKSHP)
                    seamark.shp = BEACON;
                  break;
                case BOYCAR:
                case BOYISD:
                case BOYLAT:
                case BOYSAW:
                case BOYSPP:
                  if (seamark.shp == UNKSHP)
                    seamark.shp = PILLAR;
                  break;
                default:
                  break;
              }
              
              /* Save body shape & category attributes, convert body colour attributes */
              
              switch (getkey((Map_t*)AttSTR, attribute)) {
                case SHAPE:
                  seamark.shp = (Shp_t)getkey((Map_t*)ShpSTR, value);
                  break;
                case COLOUR:
                  item = strtok(value, ";");
                  bzero(str, sizeof(str));
                  do {
                    Col_t col = (Col_t)getkey((Map_t*)ColSTR, item);
                    strcat(str, getval((Map_t*)ColMAP, (Key_t)col));
                    item = strtok(NULL, ";");
                  } while (item != NULL);
                  printf("<tag k=\"seamark:body_colour\" v=\"%s\"/>\n", str);
                  break;
                case CATEGORY:
                  seamark.cat = (Cat_t)getkey((Map_t*)CatSTR, value);
                  break;
                case COLPAT:
                  printf("<tag k=\"seamark:body_colour_pattern\" v=\"%s\"/>\n", value);
                  break;
                case SYSTEM:
                  break;
                default:
                  printf("%s", line);
              }
              break;
              
            case LNDMRK:
              if (getkey((Map_t*)AttSTR, attribute) == CATEGORY) {
                seamark.shp = (Shp_t)getkey((Map_t*)ShpSTR, value);
                if (seamark.shp == TOWER)
                  seamark.shp = LANDTOWER;
              }
              break;
            case OFSPLF:
              if ((getkey((Map_t*)AttSTR, attribute) == CATEGORY) && (strcmp(value, "fpso") == 0))
                seamark.shp = FPSO;
              break;
            default:
              printf("%s", line);
              break;
          }
        }
      }
    } else {
      
      /* Not a tag */
      printf("%s", line);
    }
	}
  return 0;
}

/* Construct light strings for individual sectors */

void lightstring(int idx, char *key) {
  int i, j;
  int colours = 0;
  for (i = group[idx].matching, j = 0; i != 0; i >>= 1, j++) {
    if (i & 1) {
      colours |= seamark.lgt[j].col;
    }
  }
  for (i = group[idx].matching, j = 0; (i & 1) == 0; i >>= 1, j++){}
  strcpy(str, "");
  if ((seamark.lgt[j].lit == DIR) || (seamark.lgt[0].lit == DIR))
    strcat(str, "Dir.");
  if (strlen(seamark.lgt[j].chr) > 0) {
    if (strcmp(seamark.lgt[j].chr, "Al") == 0)
      strcat(str, "F");
    else if (strncmp(seamark.lgt[j].chr, "Al.", 3) == 0)
      strcat(str, &seamark.lgt[j].chr[3]);
    else
      strcat(str, seamark.lgt[j].chr);
  } else {
    strcat(str, seamark.lgt[0].chr);
  }
  if (strlen(seamark.lgt[j].grp) > 0) {
    sprintf(strchr(str, 0), "(%s)", seamark.lgt[j].grp);
  } else if (strlen(seamark.lgt[0].grp) > 0) {
    sprintf(strchr(str, 0), "(%s)", seamark.lgt[0].grp);
  } else
    strcat(str, ".");
  for (i = 1; i < 256; i <<= 1) {
    if (i & colours)
      strcat(str, getval((Map_t*)ColMAP, (Key_t)i));
  }
  if (seamark.lgt[j].per > 0)
    sprintf(strchr(str, 0), " %ds", seamark.lgt[j].per);
  else if (seamark.lgt[0].per > 0)
    sprintf(strchr(str, 0), " %ds", seamark.lgt[0].per);
  if (seamark.lgt[j].hgt > 0)
    sprintf(strchr(str, 0), " %dm", seamark.lgt[j].hgt);
  else if (seamark.lgt[0].hgt > 0)
    sprintf(strchr(str, 0), " %dm", seamark.lgt[0].hgt);
  if (seamark.lgt[j].rng > 0)
    sprintf(strchr(str, 0), " %dM", seamark.lgt[j].rng);
  else if (seamark.lgt[0].rng > 0)
    sprintf(strchr(str, 0), " %dM", seamark.lgt[0].rng);
  printf("    <tag k=\"%s\" v=\"%s\"/>\n", key, str);  
}