add_subdirectory(vim)

if(SWIFT_INSTALL_TOOLS)
  # FIXME: install gyb.
endif()

swift_install_in_component(editor-integration
    FILES swift-mode.el
    DESTINATION "share/emacs/site-lisp")

