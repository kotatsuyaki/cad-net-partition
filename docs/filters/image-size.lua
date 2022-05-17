function Image(elem)
    local attr = elem.attr
    attr.attributes['width'] = '55%'
    return pandoc.Image(elem.caption, elem.src, elem.title, attr)
end
