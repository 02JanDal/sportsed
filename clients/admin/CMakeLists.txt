add_ui_files(GUI_SRC MainWindow CompetitionEditWidget CoursesEditWidget ClassesEditWidget CompetitorsEditWidget)
prepend_to_list(GUI_SRC gui/)

set(MODELS_SRC
	ClientModel.h
	ClientModel.cpp
	StageModel.h
	StageModel.cpp
	CourseModel.h
	CourseModel.cpp
	ControlModel.h
	ControlModel.cpp
	ClassModel.h
	ClassModel.cpp
	CompetitorModel.h
	CompetitorModel.cpp
)
prepend_to_list(MODELS_SRC models/)

add_client(GUI NAME admin DISPLAY_NAME Admin NAMESPACE Admin SRCS ${GUI_SRC} ${MODELS_SRC})
