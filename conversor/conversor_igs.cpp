#include <STEPControl_Reader.hxx>
#include <IGESControl_Controller.hxx>
#include <IGESControl_Writer.hxx>
#include <TopoDS_Shape.hxx>
#include <BRep_Builder.hxx>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " entrada.step saida.igs\n";
        return 1;
    }

    std::string stepFile = argv[1];
    std::string igesFile = argv[2];

    STEPControl_Reader reader;
    IFSelect_ReturnStatus status = reader.ReadFile(stepFile.c_str());

    if (status != IFSelect_RetDone) {
        std::cerr << "Erro ao ler o arquivo STEP: " << stepFile << "\n";
        return 1;
    }

    reader.TransferRoots();
    TopoDS_Shape shape = reader.OneShape();

    IGESControl_Controller::Init();
    IGESControl_Writer writer("MM", 0);
    writer.AddShape(shape);
    writer.Write(igesFile.c_str());

    std::cout << "Conversão concluída: " << igesFile << "\n";
    return 0;
}
