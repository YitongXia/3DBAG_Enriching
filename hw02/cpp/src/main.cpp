
#include <iostream>
#include <fstream>
#include <string>
#include "json.hpp"

using json = nlohmann::json;


int   get_no_roof_surfaces(json &j);
void  list_all_vertices(json& j);
void  visit_roofsurfaces(json &j);

void cal_floor(json & j)
{
    for(auto& co:j["CityObjects"].items()) {
        std::cout << co.value()["type"] << std::endl;
        if (co.value()["type"] == "Building") {
            std::cout<<co.value()["attributes"]<<std::endl;
            for (auto &g: co.value()) {
                std::cout<<g<<std::endl;
                if(g["h_dak_max"]== nullptr || g["h_dak_min"]== nullptr ||g["h_maaiveld"]== nullptr )
                    break;
                else {
                    double h_dak_max = g["h_dak_max"];
                    double h_dak_min = g["h_dak_min"];
                    double h_maaiveld = g["h_maaiveld"];
                    double height = (h_dak_max - h_dak_min) * 0.7 + (h_dak_min - h_maaiveld);
                    double floor=height/3;
                    if(floor>=int(height/3)+0.5) floor=int(height/3)+1;
                    else if(floor<int(height/3)+0.5) floor=int(height/3);
                    g.push_back({"floor:", floor});
                    break;
                }
            }
        }
    }
}

int main(int argc, const char * argv[]) {

  //-- reading the file with nlohmann json: https://github.com/nlohmann/json  
  std::ifstream input("../../data/twobuildings.city.json");
  json j;
  input >> j;
  input.close();

  //-- get total number of RoofSurface in the file
  // int noroofsurfaces = get_no_roof_surfaces(j);
  // std::cout << "Total RoofSurface: " << noroofsurfaces << std::endl;

  // list_all_vertices(j);

  visit_roofsurfaces(j);

  //-- print out the number of Buildings in the file
  int nobuildings = 0;
  for (auto& co : j["CityObjects"]) {
    if (co["type"] == "Building") {
      nobuildings += 1;
    }
  }
  std::cout << "There are " << nobuildings << " Buildings in the file" << std::endl;

  //-- print out the number of vertices in the file
  std::cout << "Number of vertices " << j["vertices"].size() << std::endl;

  //-- add an attribute "volume"
  for (auto& co : j["CityObjects"]) {
    if (co["type"] == "Building") {
      co["attributes"]["volume"] = rand();
    }
  }

  //-- write to disk the modified city model (myfile.city.json)
  std::ofstream o("myfile.city.json");
  o << j.dump(2) << std::endl;
  o.close();

  return 0;
}


// Visit every 'RoofSurface' in the CityJSON model and output its geometry (the arrays of indices)
// Useful to learn to visit the geometry boundaries and at the same time check their semantics.
void visit_roofsurfaces(json &j) {
  for (auto& co : j["CityObjects"].items()) {
    for (auto& g : co.value()["geometry"]) {
      if (g["type"] == "Solid") {
        for (int i = 0; i < g["boundaries"].size(); i++) {
          for (int j = 0; j < g["boundaries"][i].size(); j++) {
            int sem_index = g["semantics"]["values"][i][j];
            if (g["semantics"]["surfaces"][sem_index]["type"].get<std::string>().compare("RoofSurface") == 0) {
              std::cout << "RoofSurface: " << g["boundaries"][i][j] << std::endl;
            }
          }
        }
      }
    }
  }
}


// Returns the number of 'RooSurface' in the CityJSON model
int get_no_roof_surfaces(json &j) {
  int total = 0;
  for (auto& co : j["CityObjects"].items()) {
    for (auto& g : co.value()["geometry"]) {
      if (g["type"] == "Solid") {
        for (auto& shell : g["semantics"]["values"]) {
          for (auto& s : shell) {
            if (g["semantics"]["surfaces"][s.get<int>()]["type"].get<std::string>().compare("RoofSurface") == 0) {
              total += 1;
            }
          }
        }
      }
    }
  }
  return total;
}


// CityJSON files have their vertices compressed: https://www.cityjson.org/specs/1.1.1/#transform-object
// this function visits all the surfaces and print the (x,y,z) coordinates of each vertex encountered
void list_all_vertices(json& j) {
  for (auto& co : j["CityObjects"].items()) {
    std::cout << "= CityObject: " << co.key() << std::endl;
    for (auto& g : co.value()["geometry"]) {
      if (g["type"] == "Solid") {
        for (auto& shell : g["boundaries"]) {
          for (auto& surface : shell) {
            for (auto& ring : surface) {
              std::cout << "---" << std::endl;
              for (auto& v : ring) { 
                std::vector<int> vi = j["vertices"][v.get<int>()];
                double x = (vi[0] * j["transform"]["scale"][0].get<double>()) + j["transform"]["translate"][0].get<double>();
                double y = (vi[1] * j["transform"]["scale"][1].get<double>()) + j["transform"]["translate"][1].get<double>();
                double z = (vi[2] * j["transform"]["scale"][2].get<double>()) + j["transform"]["translate"][2].get<double>();
                std::cout << std::setprecision(2) << std::fixed << v << " (" << x << ", " << y << ", " << z << ")" << std::endl;                
              }
            }
          }
        }
      }
    }
  }
}
