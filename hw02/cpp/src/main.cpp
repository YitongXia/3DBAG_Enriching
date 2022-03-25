
#include <iostream>
#include <fstream>
#include <string>
#include "json.hpp"
#include <cmath>

using json = nlohmann::json;

class Point{
public:
    double x;
    double y;
    double z;
    Point(double x1,double y1,double z1): x(x1),y(y1),z(z1)
    {}
};


void visit_area(json &j) {
    for (auto& co : j["CityObjects"].items()) {
        for (auto& g : co.value()["geometry"]) {
            if (g["type"] == "Solid") {
                for(auto & surface:g["semantics"]["surfaces"]){
                    std::cout<<surface<<std::endl;
                }
            }
        }
    }
}


// function to calculate the number of floor.
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



double triangle_area(std::vector<Point> &v) {
    double a= sqrt((v[0].x-v[1].x)*(v[0].x-v[1].x)+(v[0].y-v[1].y)*(v[0].y-v[1].y) +(v[0].z-v[1].z)*(v[0].z-v[1].z));
    double b= sqrt((v[1].x-v[2].x)*(v[1].x-v[2].x)+(v[1].y-v[2].y)*(v[1].y-v[2].y) +(v[1].z-v[2].z)*(v[1].z-v[2].z));
    double c= sqrt((v[2].x-v[0].x)*(v[2].x-v[0].x)+(v[2].y-v[0].y)*(v[2].y-v[0].y) +(v[2].z-v[0].z)*(v[2].z-v[0].z));
    double s=(a+b+c)/2;
    double area= sqrt(s*(s-a)*(s-b)*(s-c));
    return area;
}


 void split_surface(json &j ){

     for (auto& co : j["CityObjects"].items()) {
         for (auto &g: co.value()["geometry"]) {

             if (g["type"] == "Solid") {
                 int roof_index = 0;
                 int surface_index = 0;
                 for (int i = 0; i < g["semantics"]["surfaces"].size(); ++i) {
                     if (g["semantics"]["surfaces"][i]["type"] == "RoofSurface")
                     {
                         roof_index = i;
                         surface_index++;
                     }
                     else surface_index++;
                 }
                 for (auto &values: g["semantics"]["values"]) {

                     for (int k = 0; k < values.size(); ++k) {
                         if (values[k] == roof_index) {
                             for (auto &shell: g["boundaries"]) {
                                 g["semantics"]["surfaces"][surface_index]["type"] = "RoofSurface";
                                 g["semantics"]["surfaces"][surface_index]["area"] = 0.0;
                                 g["semantics"]["surfaces"][surface_index]["BuildingPart_id"] = co.key();
                                 values[k] = surface_index;
                                 surface_index++;
                             }
                         }
                     }
                 }
             }
         }
     }
}




void cal_area(json & j, json &tri_j) {

    for (auto &co: tri_j["CityObjects"].items()) {
        int roofsurface = 0;

        for (auto &g: co.value()["geometry"]) {
            std::vector<int> roof;
            std::vector<double> area_value;

            if (g["type"] == "Solid") {

                std::string index=co.key();

                for (int i = 0; i < g["semantics"]["surfaces"].size(); ++i) {
                    //std::cout << g["semantics"]["surfaces"][i] << std::endl;
                    if (g["semantics"]["surfaces"][i]["type"] == "RoofSurface" && g["semantics"]["surfaces"][i]["area"] == 0.0) {
                        roof.emplace_back(i);
                    }
                }
                for(auto &item: roof){
                    area_value.emplace_back(0.0);
                }
                for (auto &ring: g["semantics"]["values"]) {

                    for (int i = 0; i < ring.size(); ++i) {

                        for (int k = 0; k < roof.size(); ++k) {

                            if (ring[i] == roof[k]) {

                                for (auto &shell: g["boundaries"]) {
                                    for (auto &surface: shell[i]) {
                                        std::vector<Point> vertices;

                                        for (auto &v: surface) {

                                            std::vector<int> vi = tri_j["vertices"][v.get<int>()];
                                            double x = (vi[0] * tri_j["transform"]["scale"][0].get<double>()) +
                                                       tri_j["transform"]["translate"][0].get<double>();
                                            double y = (vi[1] * tri_j["transform"]["scale"][1].get<double>()) +
                                                       tri_j["transform"]["translate"][1].get<double>();
                                            double z = (vi[2] * tri_j["transform"]["scale"][2].get<double>()) +
                                                       tri_j["transform"]["translate"][2].get<double>();
                                            vertices.emplace_back(Point(x, y, z));
                                        }
                                        double area = triangle_area(vertices);
                                        area_value[k]+=area;

                                    }
                                }
                            }
                        }
                    }
                }
                for(auto & g1: j["CityObjects"][index]["geometry"].items()){

                    for(int p=0;p<roof.size();++p)
                    {
                        int roof_index=roof[p];
                        g1.value()["semantics"]["surfaces"][roof_index]["area"] = area_value[p];

                    }
                }
            }
        }
    }
}

int main(int argc, const char * argv[]) {

    //-- reading the file with nlohmann json: https://github.com/nlohmann/json

    // input file
    // for the first step: create new semantic object:
    std::ifstream input("../../data/myfile.city.json");
    json j;
    input >> j;
    input.close();
    
    std::ofstream o("../../data/split_myfile.city.json");
    o << j.dump(2) << std::endl;
    o.close();
    
    // then triangulate the split_myfile.city.json
    // and name the triangulated file as tri_split_myfile.city.json
    // and use the code below to calculate the area and output file.
    //don't forget to comment the code above and uncomment the code below!
    
//    std::ifstream split_input("../../data/split_myfile.city.json");
//    json split_j;
//    split_input >> split_j;
//    split_input.close();
//
//    std::ifstream tri("../../data/tri_split_myfile.city.json");
//    json tri_j;
//    tri >> tri_j;
//    tri.close();
//
//    //split_surface(j);
//    cal_area(j,tri_j);
//
//    std::ofstream o1("../../data/final_cal.city.json");
//    o1 << j.dump(2) << std::endl;
//    o1.close();

    return 0;
}
